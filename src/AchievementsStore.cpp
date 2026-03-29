#include "AchievementsStore.h"

#include <Arduino.h>
#include <Epub.h>
#include <HalStorage.h>
#include <I18n.h>
#include <JsonSettingsIO.h>

#include <algorithm>
#include <ctime>

#include "CrossPointSettings.h"
#include "activities/reader/BookmarkStore.h"
#include "util/TimeUtils.h"

namespace {
constexpr char ACHIEVEMENTS_FILE_JSON[] = "/.crosspoint/achievements.json";

uint32_t countGoalDaysFromStats() {
  uint32_t count = 0;
  for (const auto& day : READING_STATS.getReadingDays()) {
    if (day.readingMs >= getDailyReadingGoalMs()) {
      ++count;
    }
  }
  return count;
}

uint32_t countSessionsFromStats() {
  uint32_t count = 0;
  for (const auto& book : READING_STATS.getBooks()) {
    count += book.sessions;
  }
  return count;
}

uint32_t countCurrentBookmarksFromStats() {
  uint32_t count = 0;
  for (const auto& book : READING_STATS.getBooks()) {
    if (book.path.empty() || !Storage.exists(book.path.c_str())) {
      continue;
    }

    Epub epub(book.path, "/.crosspoint");
    BookmarkStore store;
    store.load(epub.getCachePath());
    count += static_cast<uint32_t>(store.getAll().size());
  }
  return count;
}

uint32_t findLongestSessionFromStats() {
  uint32_t maxSessionMs = 0;
  for (const auto& book : READING_STATS.getBooks()) {
    maxSessionMs = std::max(maxSessionMs, book.lastSessionMs);
  }
  return maxSessionMs;
}
}  // namespace

AchievementsStore AchievementsStore::instance;

const std::array<AchievementDefinition, static_cast<size_t>(AchievementId::_COUNT)>& AchievementsStore::definitions() {
  static const std::array<AchievementDefinition, static_cast<size_t>(AchievementId::_COUNT)> items = {
      AchievementDefinition{AchievementId::FirstBookStarted, AchievementMetric::BooksStarted, 1, "Open Sesame",
                            "Abrete libro", "Start your first book.", "Empieza tu primer libro."},
      AchievementDefinition{AchievementId::FiveBooksStarted, AchievementMetric::BooksStarted, 5, "Collector",
                            "Coleccionista", "Start 5 different books.", "Empieza 5 libros distintos."},
      AchievementDefinition{AchievementId::TenBooksStarted, AchievementMetric::BooksStarted, 10, "Shelf Diver",
                            "Buceador de estanterias", "Start 10 different books.",
                            "Empieza 10 libros distintos."},
      AchievementDefinition{AchievementId::TwentyFiveBooksStarted, AchievementMetric::BooksStarted, 25, "Book Hopper",
                            "Saltalibros", "Start 25 different books.", "Empieza 25 libros distintos."},
      AchievementDefinition{AchievementId::FiftyBooksStarted, AchievementMetric::BooksStarted, 50, "Library Tourist",
                            "Turista de biblioteca", "Start 50 different books.",
                            "Empieza 50 libros distintos."},
      AchievementDefinition{AchievementId::FirstSession, AchievementMetric::Sessions, 1, "Warm-Up",
                            "Calentamiento", "Complete your first counted session.",
                            "Completa tu primera sesion valida."},
      AchievementDefinition{AchievementId::TenSessions, AchievementMetric::Sessions, 10, "Page Ritual",
                            "Ritual lector", "Complete 10 counted sessions.", "Completa 10 sesiones validas."},
      AchievementDefinition{AchievementId::TwentyFiveSessions, AchievementMetric::Sessions, 25, "Session Machine",
                            "Maquina de sesiones", "Complete 25 counted sessions.",
                            "Completa 25 sesiones validas."},
      AchievementDefinition{AchievementId::FiftySessions, AchievementMetric::Sessions, 50, "Unstoppable",
                            "Imparable", "Complete 50 counted sessions.", "Completa 50 sesiones validas."},
      AchievementDefinition{AchievementId::OneHundredSessions, AchievementMetric::Sessions, 100, "Century Sessions",
                            "Cien sesiones", "Complete 100 counted sessions.",
                            "Completa 100 sesiones validas."},
      AchievementDefinition{AchievementId::TwoHundredSessions, AchievementMetric::Sessions, 200, "Routine Master",
                            "Maestro del ritmo", "Complete 200 counted sessions.",
                            "Completa 200 sesiones validas."},
      AchievementDefinition{AchievementId::FirstBookFinished, AchievementMetric::BooksFinished, 1, "The End",
                            "Fin", "Finish your first book.", "Termina tu primer libro."},
      AchievementDefinition{AchievementId::TwoBooksFinished, AchievementMetric::BooksFinished, 2, "Belle of the Books",
                            "Bella entre libros", "Finish 2 books.", "Termina 2 libros."},
      AchievementDefinition{AchievementId::ThreeBooksFinished, AchievementMetric::BooksFinished, 3, "Trilogy",
                            "Trilogia", "Finish 3 books.", "Termina 3 libros."},
      AchievementDefinition{AchievementId::FiveBooksFinished, AchievementMetric::BooksFinished, 5, "Finish Line",
                            "Meta lectora", "Finish 5 books.", "Termina 5 libros."},
      AchievementDefinition{AchievementId::SevenBooksFinished, AchievementMetric::BooksFinished, 7, "Word Warden",
                            "Guardian de las palabras", "Finish 7 books.", "Termina 7 libros."},
      AchievementDefinition{AchievementId::TenBooksFinished, AchievementMetric::BooksFinished, 10, "Top Shelf",
                            "Estanteria top", "Finish 10 books.", "Termina 10 libros."},
      AchievementDefinition{AchievementId::FifteenBooksFinished, AchievementMetric::BooksFinished, 15, "Ink Tamer",
                            "Domador de tinta", "Finish 15 books.", "Termina 15 libros."},
      AchievementDefinition{AchievementId::TwentyBooksFinished, AchievementMetric::BooksFinished, 20, "Closing Time",
                            "Hora del cierre", "Finish 20 books.", "Termina 20 libros."},
      AchievementDefinition{AchievementId::TwentyFiveBooksFinished, AchievementMetric::BooksFinished, 25,
                            "Beast of the Library", "Bestia de biblioteca", "Finish 25 books.",
                            "Termina 25 libros."},
      AchievementDefinition{AchievementId::ThirtyBooksFinished, AchievementMetric::BooksFinished, 30,
                            "Chapter Collector", "Coleccionista de capitulos", "Finish 30 books.",
                            "Termina 30 libros."},
      AchievementDefinition{AchievementId::FortyBooksFinished, AchievementMetric::BooksFinished, 40, "Story Keeper",
                            "Guardian de historias", "Finish 40 books.", "Termina 40 libros."},
      AchievementDefinition{AchievementId::FiftyBooksFinished, AchievementMetric::BooksFinished, 50, "Fifty Tomes",
                            "Cincuenta tomos", "Finish 50 books.", "Termina 50 libros."},
      AchievementDefinition{AchievementId::SixtyBooksFinished, AchievementMetric::BooksFinished, 60,
                            "Library Knight", "Caballero de biblioteca", "Finish 60 books.",
                            "Termina 60 libros."},
      AchievementDefinition{AchievementId::SeventyFiveBooksFinished, AchievementMetric::BooksFinished, 75,
                            "Lord of the Shelves", "Senor de las estanterias", "Finish 75 books.",
                            "Termina 75 libros."},
      AchievementDefinition{AchievementId::OneHundredBooksFinished, AchievementMetric::BooksFinished, 100,
                            "Master of a Hundred Tales", "Maestro de cien relatos", "Finish 100 books.",
                            "Termina 100 libros."},
      AchievementDefinition{AchievementId::ReadingOneHour, AchievementMetric::TotalReadingMs, 60ULL * 60ULL * 1000ULL,
                            "One-Hour Club", "Club de la hora", "Read for 1 hour in total.",
                            "Lee 1 hora en total."},
      AchievementDefinition{AchievementId::ReadingFiveHours, AchievementMetric::TotalReadingMs,
                            5ULL * 60ULL * 60ULL * 1000ULL, "Five and Rising", "Cinco y subiendo",
                            "Read for 5 hours in total.", "Lee 5 horas en total."},
      AchievementDefinition{AchievementId::ReadingTenHours, AchievementMetric::TotalReadingMs,
                            10ULL * 60ULL * 60ULL * 1000ULL, "Tenacious Reader", "Lectura tenaz",
                            "Read for 10 hours in total.", "Lee 10 horas en total."},
      AchievementDefinition{AchievementId::ReadingOneDay, AchievementMetric::TotalReadingMs,
                            24ULL * 60ULL * 60ULL * 1000ULL, "Full Day", "Dia redondo",
                            "Spend a full day reading in total.", "Suma un dia entero de lectura."},
      AchievementDefinition{AchievementId::ReadingFiftyHours, AchievementMetric::TotalReadingMs,
                            50ULL * 60ULL * 60ULL * 1000ULL, "Fifty Forward", "Cincuenta horas",
                            "Read for 50 hours in total.", "Lee 50 horas en total."},
      AchievementDefinition{AchievementId::ReadingOneHundredHours, AchievementMetric::TotalReadingMs,
                            100ULL * 60ULL * 60ULL * 1000ULL, "Century Reader", "Lector centenario",
                            "Read for 100 hours in total.", "Lee 100 horas en total."},
      AchievementDefinition{AchievementId::ReadingTwoHundredHours, AchievementMetric::TotalReadingMs,
                            200ULL * 60ULL * 60ULL * 1000ULL, "Double Century", "Doble centuria",
                            "Read for 200 hours in total.", "Lee 200 horas en total."},
      AchievementDefinition{AchievementId::FirstGoalDay, AchievementMetric::GoalDays, 1, "Goal Getter",
                            "Meta cumplida", "Reach the daily goal once.", "Cumple la meta diaria una vez."},
      AchievementDefinition{AchievementId::SevenGoalDays, AchievementMetric::GoalDays, 7, "Goal Habit",
                            "Habito de meta", "Reach the daily goal on 7 days.", "Cumple la meta diaria 7 dias."},
      AchievementDefinition{AchievementId::ThirtyGoalDays, AchievementMetric::GoalDays, 30, "Goal Season",
                            "Temporada de metas", "Reach the daily goal on 30 days.",
                            "Cumple la meta diaria 30 dias."},
      AchievementDefinition{AchievementId::SixtyGoalDays, AchievementMetric::GoalDays, 60, "Goal Calendar",
                            "Calendario de metas", "Reach the daily goal on 60 days.",
                            "Cumple la meta diaria 60 dias."},
      AchievementDefinition{AchievementId::ThreeGoalStreak, AchievementMetric::MaxGoalStreak, 3, "Three in a Row",
                            "Tres al hilo", "Reach a 3-day goal streak.", "Consigue una racha de meta de 3 dias."},
      AchievementDefinition{AchievementId::SevenGoalStreak, AchievementMetric::MaxGoalStreak, 7, "Week Locked",
                            "Semana blindada", "Reach a 7-day goal streak.",
                            "Consigue una racha de meta de 7 dias."},
      AchievementDefinition{AchievementId::FourteenGoalStreak, AchievementMetric::MaxGoalStreak, 14, "Fortnight Fire",
                            "Quincena encendida", "Reach a 14-day goal streak.",
                            "Consigue una racha de meta de 14 dias."},
      AchievementDefinition{AchievementId::ThirtyGoalStreak, AchievementMetric::MaxGoalStreak, 30, "Month Boss",
                            "Jefe del mes", "Reach a 30-day goal streak.",
                            "Consigue una racha de meta de 30 dias."},
      AchievementDefinition{AchievementId::SixtyGoalStreak, AchievementMetric::MaxGoalStreak, 60, "Season Boss",
                            "Jefe de temporada", "Reach a 60-day goal streak.",
                            "Consigue una racha de meta de 60 dias."},
      AchievementDefinition{AchievementId::FirstBookmark, AchievementMetric::TotalBookmarksAdded, 1, "Pin It",
                            "Fijalo", "Add your first bookmark.", "Anade tu primer marcador."},
      AchievementDefinition{AchievementId::TenBookmarks, AchievementMetric::TotalBookmarksAdded, 10,
                            "Bookmark Hoarder", "Acumulador de marcadores", "Add 10 bookmarks.",
                            "Anade 10 marcadores."},
      AchievementDefinition{AchievementId::TwentyFiveBookmarks, AchievementMetric::TotalBookmarksAdded, 25,
                            "Flag Garden", "Jardin de banderas", "Add 25 bookmarks.",
                            "Anade 25 marcadores."},
      AchievementDefinition{AchievementId::FiftyBookmarks, AchievementMetric::TotalBookmarksAdded, 50,
                            "Flagstorm", "Tormenta de marcadores", "Add 50 bookmarks.",
                            "Anade 50 marcadores."},
      AchievementDefinition{AchievementId::FifteenMinuteSession, AchievementMetric::MaxSessionMs,
                            15ULL * 60ULL * 1000ULL, "Settled In", "Ya en ritmo",
                            "Complete a 15-minute session.", "Completa una sesion de 15 minutos."},
      AchievementDefinition{AchievementId::ThirtyMinuteSession, AchievementMetric::MaxSessionMs,
                            30ULL * 60ULL * 1000ULL, "Deep Dive", "Inmersion",
                            "Complete a 30-minute session.", "Completa una sesion de 30 minutos."},
      AchievementDefinition{AchievementId::FortyFiveMinuteSession, AchievementMetric::MaxSessionMs,
                            45ULL * 60ULL * 1000ULL, "Locked In", "Enfocado",
                            "Complete a 45-minute session.", "Completa una sesion de 45 minutos."},
      AchievementDefinition{AchievementId::SixtyMinuteSession, AchievementMetric::MaxSessionMs,
                            60ULL * 60ULL * 1000ULL, "Hourglass", "Reloj de arena",
                            "Complete a 60-minute session.", "Completa una sesion de 60 minutos."},
      AchievementDefinition{AchievementId::NinetyMinuteSession, AchievementMetric::MaxSessionMs,
                            90ULL * 60ULL * 1000ULL, "Marathon", "Maraton",
                            "Complete a 90-minute session.", "Completa una sesion de 90 minutos."},
      AchievementDefinition{AchievementId::TwoHourSession, AchievementMetric::MaxSessionMs,
                            120ULL * 60ULL * 1000ULL, "Ultra Session", "Sesion ultra",
                            "Complete a 120-minute session.", "Completa una sesion de 120 minutos."},
  };
  return items;
}

uint32_t AchievementsStore::getReferenceTimestamp() {
  bool usedFallback = false;
  const uint32_t timestamp = READING_STATS.getDisplayTimestamp(&usedFallback);
  return TimeUtils::isClockValid(timestamp) ? timestamp : static_cast<uint32_t>(time(nullptr));
}

bool AchievementsStore::hasString(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

void AchievementsStore::markDirty() { dirty = true; }

std::string AchievementsStore::getTitle(const AchievementId id) const {
  const auto& definition = getDefinition(id);
  return I18N.getLanguage() == Language::ES ? definition.titleEs : definition.titleEn;
}

std::string AchievementsStore::getDescription(const AchievementId id) const {
  const auto& definition = getDefinition(id);
  return I18N.getLanguage() == Language::ES ? definition.descriptionEs : definition.descriptionEn;
}

void AchievementsStore::unlock(const AchievementId id, const uint32_t timestamp, const bool enqueuePopup) {
  auto& state = states[indexOf(id)];
  if (state.unlocked) {
    return;
  }

  state.unlocked = true;
  state.unlockedAt = timestamp;
  if (enqueuePopup && SETTINGS.achievementPopups) {
    pendingUnlocks.push_back(id);
  }
  markDirty();
}

uint64_t AchievementsStore::getEffectiveTodayReadingMs(const uint32_t dayOrdinal) const {
  uint64_t todayReadingMs = READING_STATS.getTodayReadingMs();
  if (dayOrdinal != 0 && dayOrdinal == resetDayOrdinal) {
    if (todayReadingMs > resetDayBaselineMs) {
      todayReadingMs -= resetDayBaselineMs;
    } else {
      todayReadingMs = 0;
    }
  }
  return todayReadingMs;
}

AchievementsStore::ProgressSnapshot AchievementsStore::buildProgressSnapshot() const {
  ProgressSnapshot snapshot;
  snapshot.booksStarted = static_cast<uint32_t>(startedBooks.size());
  snapshot.booksFinished = static_cast<uint32_t>(finishedBooks.size());
  snapshot.sessions = countedSessions;
  snapshot.totalReadingMs = accumulatedReadingMs;
  snapshot.goalDays = goalDaysCount;
  snapshot.maxGoalStreak = maxGoalStreak;
  snapshot.totalBookmarksAdded = totalBookmarksAdded;
  snapshot.maxSessionMs = longestSessionMs;
  return snapshot;
}

void AchievementsStore::evaluateProgress(const bool enqueuePopups) {
  const auto progress = buildProgressSnapshot();
  const uint32_t unlockTimestamp = getReferenceTimestamp();

  for (const auto& definition : definitions()) {
    uint64_t currentValue = 0;
    switch (definition.metric) {
      case AchievementMetric::BooksStarted:
        currentValue = progress.booksStarted;
        break;
      case AchievementMetric::BooksFinished:
        currentValue = progress.booksFinished;
        break;
      case AchievementMetric::Sessions:
        currentValue = progress.sessions;
        break;
      case AchievementMetric::TotalReadingMs:
        currentValue = progress.totalReadingMs;
        break;
      case AchievementMetric::GoalDays:
        currentValue = progress.goalDays;
        break;
      case AchievementMetric::MaxGoalStreak:
        currentValue = progress.maxGoalStreak;
        break;
      case AchievementMetric::TotalBookmarksAdded:
        currentValue = progress.totalBookmarksAdded;
        break;
      case AchievementMetric::MaxSessionMs:
        currentValue = progress.maxSessionMs;
        break;
    }

    if (currentValue >= definition.target) {
      unlock(definition.id, unlockTimestamp, enqueuePopups);
    }
  }
}

void AchievementsStore::bootstrapFromCurrentStats() {
  startedBooks.clear();
  finishedBooks.clear();

  for (const auto& book : READING_STATS.getBooks()) {
    if (book.path.empty()) {
      continue;
    }
    startedBooks.push_back(book.path);
    if (book.completed) {
      finishedBooks.push_back(book.path);
    }
  }

  accumulatedReadingMs = READING_STATS.getTotalReadingMs();
  countedSessions = countSessionsFromStats();
  totalBookmarksAdded = countCurrentBookmarksFromStats();
  longestSessionMs = findLongestSessionFromStats();
  resetDayOrdinal = 0;
  resetDayBaselineMs = 0;
  refreshGoalDerivedProgressFromStats();

  evaluateProgress(false);
  markDirty();
}

bool AchievementsStore::refreshGoalDerivedProgressFromStats() {
  const uint32_t newGoalDaysCount = countGoalDaysFromStats();
  const uint32_t newCurrentGoalStreak = READING_STATS.getCurrentStreakDays();
  const uint32_t newMaxGoalStreak = READING_STATS.getMaxStreakDays();

  uint32_t newLastGoalDayOrdinal = 0;
  for (const auto& day : READING_STATS.getReadingDays()) {
    if (day.readingMs >= getDailyReadingGoalMs()) {
      newLastGoalDayOrdinal = day.dayOrdinal;
    }
  }

  const bool changed = goalDaysCount != newGoalDaysCount || currentGoalStreak != newCurrentGoalStreak ||
                       maxGoalStreak != newMaxGoalStreak || lastGoalDayOrdinal != newLastGoalDayOrdinal;

  goalDaysCount = newGoalDaysCount;
  currentGoalStreak = newCurrentGoalStreak;
  maxGoalStreak = newMaxGoalStreak;
  lastGoalDayOrdinal = newLastGoalDayOrdinal;
  return changed;
}

void AchievementsStore::reconcileFromCurrentStats() {
  if (!SETTINGS.achievementsEnabled) {
    return;
  }
  evaluateProgress(false);
  if (dirty) {
    saveToFile();
  }
}

void AchievementsStore::recordSessionEnded(const ReadingSessionSnapshot& snapshot) {
  if (!SETTINGS.achievementsEnabled || !snapshot.valid || snapshot.serial == 0 ||
      snapshot.serial == lastProcessedSessionSerial || snapshot.path.empty()) {
    return;
  }

  lastProcessedSessionSerial = snapshot.serial;

  if (!hasString(startedBooks, snapshot.path)) {
    startedBooks.push_back(snapshot.path);
    markDirty();
  }

  accumulatedReadingMs += snapshot.sessionMs;
  longestSessionMs = std::max(longestSessionMs, snapshot.sessionMs);
  if (snapshot.counted) {
    ++countedSessions;
  }

  if (snapshot.completedThisSession && !hasString(finishedBooks, snapshot.path)) {
    finishedBooks.push_back(snapshot.path);
    markDirty();
  }

  const uint32_t referenceTimestamp = getReferenceTimestamp();
  const uint32_t dayOrdinal =
      TimeUtils::isClockValid(referenceTimestamp) ? TimeUtils::getLocalDayOrdinal(referenceTimestamp) : 0;
  const uint64_t effectiveTodayReadingMs = getEffectiveTodayReadingMs(dayOrdinal);
  if (dayOrdinal != 0 && effectiveTodayReadingMs >= getDailyReadingGoalMs() && lastGoalDayOrdinal != dayOrdinal) {
    ++goalDaysCount;
    if (lastGoalDayOrdinal != 0 && lastGoalDayOrdinal + 1 == dayOrdinal) {
      ++currentGoalStreak;
    } else {
      currentGoalStreak = 1;
    }
    maxGoalStreak = std::max(maxGoalStreak, currentGoalStreak);
    lastGoalDayOrdinal = dayOrdinal;
    markDirty();
  }

  markDirty();
  evaluateProgress(true);
  saveToFile();
}

void AchievementsStore::recordBookmarkAdded() {
  if (!SETTINGS.achievementsEnabled) {
    return;
  }

  ++totalBookmarksAdded;
  markDirty();
  evaluateProgress(true);
  saveToFile();
}

std::vector<AchievementView> AchievementsStore::buildViews() const {
  const auto progress = buildProgressSnapshot();
  std::vector<AchievementView> views;
  views.reserve(definitions().size());

  for (const auto& definition : definitions()) {
    uint64_t currentValue = 0;
    switch (definition.metric) {
      case AchievementMetric::BooksStarted:
        currentValue = progress.booksStarted;
        break;
      case AchievementMetric::BooksFinished:
        currentValue = progress.booksFinished;
        break;
      case AchievementMetric::Sessions:
        currentValue = progress.sessions;
        break;
      case AchievementMetric::TotalReadingMs:
        currentValue = progress.totalReadingMs;
        break;
      case AchievementMetric::GoalDays:
        currentValue = progress.goalDays;
        break;
      case AchievementMetric::MaxGoalStreak:
        currentValue = progress.maxGoalStreak;
        break;
      case AchievementMetric::TotalBookmarksAdded:
        currentValue = progress.totalBookmarksAdded;
        break;
      case AchievementMetric::MaxSessionMs:
        currentValue = progress.maxSessionMs;
        break;
    }

    views.push_back(AchievementView{&definition, states[indexOf(definition.id)], currentValue, definition.target});
  }

  std::stable_sort(views.begin(), views.end(), [](const AchievementView& lhs, const AchievementView& rhs) {
    if (lhs.state.unlocked != rhs.state.unlocked) {
      return lhs.state.unlocked > rhs.state.unlocked;
    }
    if (lhs.state.unlocked && rhs.state.unlocked && lhs.state.unlockedAt != rhs.state.unlockedAt) {
      return lhs.state.unlockedAt > rhs.state.unlockedAt;
    }
    return false;
  });

  return views;
}

std::string AchievementsStore::popNextPopupMessage() {
  if (pendingUnlocks.empty()) {
    return "";
  }

  const AchievementId id = pendingUnlocks.front();
  pendingUnlocks.erase(pendingUnlocks.begin());
  return std::string(tr(STR_ACHIEVEMENT_UNLOCKED)) + ": " + getTitle(id);
}

bool AchievementsStore::saveToFile() const {
  if (!dirty) {
    return true;
  }

  Storage.mkdir("/.crosspoint");
  const bool saved = JsonSettingsIO::saveAchievements(*this, ACHIEVEMENTS_FILE_JSON);
  if (saved) {
    dirty = false;
  }
  return saved;
}

bool AchievementsStore::loadFromFile() {
  if (!Storage.exists(ACHIEVEMENTS_FILE_JSON)) {
    bootstrapFromCurrentStats();
    return saveToFile();
  }

  const bool loaded = JsonSettingsIO::loadAchievementsFromFile(*this, ACHIEVEMENTS_FILE_JSON);
  if (!loaded) {
    return false;
  }

  dirty = false;
  if (refreshGoalDerivedProgressFromStats()) {
    markDirty();
  }
  evaluateProgress(false);
  if (dirty) {
    saveToFile();
  }
  return true;
}

void AchievementsStore::reset() {
  states = {};
  startedBooks.clear();
  finishedBooks.clear();
  pendingUnlocks.clear();
  accumulatedReadingMs = 0;
  countedSessions = 0;
  totalBookmarksAdded = 0;
  longestSessionMs = 0;
  goalDaysCount = 0;
  currentGoalStreak = 0;
  maxGoalStreak = 0;
  lastGoalDayOrdinal = 0;
  lastProcessedSessionSerial = 0;

  const uint32_t referenceTimestamp = getReferenceTimestamp();
  if (TimeUtils::isClockValid(referenceTimestamp)) {
    resetDayOrdinal = TimeUtils::getLocalDayOrdinal(referenceTimestamp);
    resetDayBaselineMs = READING_STATS.getTodayReadingMs();
  } else {
    resetDayOrdinal = 0;
    resetDayBaselineMs = 0;
  }

  markDirty();
  saveToFile();
}

void AchievementsStore::syncWithPreviousStats() {
  for (const auto& book : READING_STATS.getBooks()) {
    if (book.path.empty()) {
      continue;
    }

    if (!hasString(startedBooks, book.path)) {
      startedBooks.push_back(book.path);
      markDirty();
    }

    if (book.completed && !hasString(finishedBooks, book.path)) {
      finishedBooks.push_back(book.path);
      markDirty();
    }
  }

  accumulatedReadingMs = std::max<uint64_t>(accumulatedReadingMs, READING_STATS.getTotalReadingMs());
  countedSessions = std::max(countedSessions, countSessionsFromStats());
  totalBookmarksAdded = std::max(totalBookmarksAdded, countCurrentBookmarksFromStats());
  longestSessionMs = std::max(longestSessionMs, findLongestSessionFromStats());
  refreshGoalDerivedProgressFromStats();

  markDirty();
  evaluateProgress(false);
  saveToFile();
}
