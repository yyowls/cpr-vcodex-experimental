# Changelog

This changelog starts at `1.2.0.24`, the point where CPR-vCodex began tracking release changes in this file.

| Version | Changes |
|---|---|
| `1.2.0.26` | - Hardened book completion tracking so achievements still register when leaving from explicit `End of book` states.<br>- Applied the completion fix consistently across EPUB, TXT, and XTC readers. |
| `1.2.0.25` | - Added PNG sleep image compatibility while keeping BMP sleep images unchanged.<br>- PNG sleep images now work as transparent overlays on top of the last visible screen.<br>- Sleep preview now recognizes and previews both BMP and PNG files. |
| `1.2.0.24` | - Reviewed and corrected all 23 bundled UI languages.<br>- Synced the fork with upstream through `64f5ef0`, including keyboard, XTC, parser, docs, font, and i18n updates.<br>- Fixed runtime UI font replacement so Vietnamese glyphs render correctly.<br>- Normalized translation newlines and multiline text handling again after the YAML/i18n regressions. |
