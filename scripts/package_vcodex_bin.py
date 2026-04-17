from __future__ import annotations

import json
import re
import shutil
from pathlib import Path

Import("env")


def extract_define(name: str) -> str:
    for define in env.get("CPPDEFINES", []):
        if isinstance(define, tuple) and len(define) >= 2 and define[0] == name:
            value = define[1]
            if isinstance(value, str):
                return value.replace('\\"', '"').strip('"')
            return str(value)
    return "unknown"


def sanitize_filename(value: str) -> str:
    sanitized = re.sub(r"[^0-9A-Za-z._+-]+", "-", value).strip(".-")
    return sanitized or "unknown"


def extract_define_int(name: str) -> int | None:
    value = extract_define(name)
    try:
        return int(value)
    except ValueError:
        return None


def package_vcodex_bin(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))
    progname = env.subst("$PROGNAME")
    project_dir = Path(env.subst("$PROJECT_DIR"))

    firmware_path = build_dir / f"{progname}.bin"
    if not firmware_path.exists():
        print(f"vcodex packaging skipped: missing {firmware_path}")
        return

    version = extract_define("CROSSPOINT_VERSION")
    build_seq = extract_define_int("VCODEX_BUILD_SEQ")
    safe_version = sanitize_filename(version)

    output_dir = project_dir / "artifacts"
    output_dir.mkdir(parents=True, exist_ok=True)

    artifact_name = f"{safe_version}-cpr-vcodex.bin"
    artifact_path = output_dir / artifact_name
    shutil.copy2(firmware_path, artifact_path)

    metadata = {
        "version": version,
        "safeVersion": safe_version,
        "artifactName": artifact_name,
        "artifactPath": str(artifact_path),
        "firmwareBytes": artifact_path.stat().st_size,
        "sourceBin": str(firmware_path),
        "environment": env.subst("$PIOENV"),
    }
    if build_seq is not None:
        metadata["buildSequence"] = build_seq
    metadata_path = output_dir / f"{safe_version}-cpr-vcodex.json"
    metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    print(f"Packaged vcodex artifact: {artifact_path}")
    print(f"Wrote vcodex metadata: {metadata_path}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", package_vcodex_bin)
