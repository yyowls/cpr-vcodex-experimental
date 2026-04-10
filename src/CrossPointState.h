#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>

class CrossPointState {
  // Static instance
  static CrossPointState instance;

 public:
  std::string openEpubPath;
  uint8_t lastSleepImage = UINT8_MAX;  // UINT8_MAX = unset sentinel
  uint8_t readerActivityLoadCount = 0;
  bool lastSleepFromReader = false;
  uint32_t lastKnownValidTimestamp = 0;
  uint8_t syncDayReminderStartCount = 0;
  bool syncDayReminderLatched = false;
  ~CrossPointState() = default;

  // Get singleton instance
  static CrossPointState& getInstance() { return instance; }

  bool saveToFile();

  bool loadFromFile();
  void recordUsefulStart(uint8_t reminderThreshold);
  void registerValidTimeSync(uint32_t validTimestamp);
  bool shouldShowSyncDayReminder(uint8_t reminderThreshold) const;

 private:
  bool loadFromBinaryFile();
};

// Helper macro to access settings
#define APP_STATE CrossPointState::getInstance()
