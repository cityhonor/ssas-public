/**
 * SSAS - Simple Smart Automotive Software
 * Copyright (C) 2022 Parai Wang <parai@foxmail.com>
 */
/* ================================ [ INCLUDES  ] ============================================== */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#include "Log.hpp"
namespace as {
/* ================================ [ MACROS    ] ============================================== */
/* ================================ [ TYPES     ] ============================================== */
/* ================================ [ DECLARES  ] ============================================== */
/* ================================ [ DATAS     ] ============================================== */
std::shared_ptr<Logger> Log::s_Logger = std::make_shared<Logger>();
/* ================================ [ LOCALS    ] ============================================== */
static float get_rel_time(void) {
  static struct timeval m0 = {-1, -1};
  struct timeval m1;
  float rtim;

  if ((-1 == m0.tv_sec) && (-1 == m0.tv_usec)) {
    gettimeofday(&m0, NULL);
  }
  gettimeofday(&m1, NULL);
  rtim = m1.tv_sec - m0.tv_sec;
  if (m1.tv_usec > m0.tv_usec) {
    rtim += (float)(m1.tv_usec - m0.tv_usec) / 1000000.0;
  } else {
    rtim = rtim - 1 + (float)(1000000.0 + m1.tv_usec - m0.tv_usec) / 1000000.0;
  }

  return rtim;
}

/* ================================ [ FUNCTIONS ] ============================================== */
Logger::Logger() {
  m_File = stdout;
}

Logger::Logger(std::string name, std::string format) {
  m_Name = name;
  m_Format = format;
  m_FileIndex = getFileIndex();
  open();
}

void Logger::setMaxSize(int sz) {
  m_FileMaxSize = sz;
}
void Logger::setMaxNum(int num) {
  m_FileMaxNum = num;
}

void Logger::setLogLevel(int level) {
  m_Level = level;
}
int Logger::getLogLevel() {
  return m_Level;
}

void Logger::open(void) {
  FILE *fp;
  std::string np;

#ifdef _WIN32
  mkdir("log");
#else
  mkdir("log", 0777);
#endif
  np = "log/" + std::string(m_Name) + "." + std::to_string(m_FileIndex) + "." + m_Format;
  fp = fopen(np.c_str(), "wb");
  if (nullptr != fp) {
    if (m_File != stdout) {
      fclose(m_File);
    }
    m_File = fp;
    setFileIndex(m_FileIndex);
    m_FileIndex++;
    if (m_FileIndex >= m_FileMaxNum) {
      m_FileIndex = 0;
    }
  } else {
    printf("FATAL: can't create log file <%s>\n", np.c_str());
    m_File = stdout;
  }
}

int Logger::getFileIndex(void) {
  int index = 0;
  FILE *fp;
  std::string np;
  np = "log/" + std::string(m_Name) + "." + "index";
  fp = fopen(np.c_str(), "rb");
  if (nullptr != fp) {
    fscanf(fp, "%d", &index);
    fclose(fp);
    index++;
    if (index >= m_FileMaxNum) {
      index = 0;
    }
  }

  return index;
}

void Logger::setFileIndex(int index) {
  FILE *fp;
  std::string np;
  np = "log/" + std::string(m_Name) + "." + "index";
  fp = fopen(np.c_str(), "wb");
  if (nullptr != fp) {
    fprintf(fp, "%d", index);
    fclose(fp);
  } else {
    printf("FATAL: can't create log index file <%s>\n", np.c_str());
  }
}

void Logger::check(void) {
  int size;

  if (m_File != stdout) {
    size = ftell(m_File);
    if (size >= m_FileMaxSize) {
      open();
    }
  }
}

void Logger::write(const char *fmt, ...) {
  va_list args;
  std::unique_lock<std::mutex> lck(m_Lock);

  va_start(args, fmt);
  (void)vfprintf(m_File, fmt, args);
  va_end(args);
}

void Logger::print(int level, const char *fmt, ...) {
  va_list args;
  std::unique_lock<std::mutex> lck(m_Lock);
  if (level >= m_Level) {
    if ((0 == memcmp(fmt, "ERROR", 5)) || (0 == memcmp(fmt, "WARN", 4)) ||
        (0 == memcmp(fmt, "INFO", 4)) || (0 == memcmp(fmt, "DEBUG", 5))) {
      float rtime = get_rel_time();
      fprintf(m_File, "%.4f ", rtime);
    }
    va_start(args, fmt);
    (void)vfprintf(m_File, fmt, args);
    va_end(args);

    check();
  }
}

void Logger::print(int level, const char *fmt, va_list args) {
  std::unique_lock<std::mutex> lck(m_Lock);
  if (level >= m_Level) {
    if ((0 == memcmp(fmt, "ERROR", 5)) || (0 == memcmp(fmt, "WARN", 4)) ||
        (0 == memcmp(fmt, "INFO", 4)) || (0 == memcmp(fmt, "DEBUG", 5))) {
      float rtime = get_rel_time();
      fprintf(m_File, "%.4f ", rtime);
    }
    (void)vfprintf(m_File, fmt, args);

    check();
  }
}

void Logger::hexdump(int level, const char *prefix, const void *data, size_t size, size_t len) {
  size_t i, j;
  uint8_t *src = (uint8_t *)data;
  uint32_t offset = 0;

  std::unique_lock<std::mutex> lck(m_Lock);

  if (size <= len) {
    len = size;
    fprintf(m_File, "%s:", prefix);
  } else {
    fprintf(m_File, "%8s:", prefix);
    for (i = 0; i < len; i++) {
      fprintf(m_File, " %02X", (uint32_t)i);
    }
    fprintf(m_File, "\n");
  }

  for (i = 0; i < (size + len - 1) / len; i++) {
    if (size > len) {
      fprintf(m_File, "%08X:", (uint32_t)offset);
    }
    for (j = 0; j < len; j++) {
      if ((i * len + j) < size) {
        fprintf(m_File, " %02X", (uint32_t)src[i * len + j]);
      } else {
        fprintf(m_File, "   ");
      }
    }
    fprintf(m_File, "\t");
    for (j = 0; j < len; j++) {
      if (((i * len + j) < size) && isprint(src[i * len + j])) {
        fprintf(m_File, "%c", src[i * len + j]);
      } else {
        fprintf(m_File, ".");
      }
    }
    fprintf(m_File, "\n");
    offset += len;
  }

  check();
}

void Logger::vprint(const char *fmt, va_list args) {
  std::unique_lock<std::mutex> lck(m_Lock);
  if (m_Ended) {
    float rtime = get_rel_time();
    fprintf(m_File, "%.4f ", rtime);
    m_Ended = false;
  }
  auto len = strlen(fmt);
  (void)vfprintf(m_File, fmt, args);
  if (fmt[len - 1] == '\n') {
    m_Ended = true;
    check();
  }
}

void Logger::putc(char chr) {
  if (m_Ended) {
    float rtime = get_rel_time();
    fprintf(m_File, "%.4f ", rtime);
    m_Ended = false;
  }
  fputc(chr, m_File);
  if (chr == '\n') {
    m_Ended = true;
  }
}

Logger::~Logger() {
  if (nullptr != m_File) {
    if (m_File != stdout) {
      fclose(m_File);
    }
  }
}

void Log::setLogLevel(int level) {
  s_Logger->setLogLevel(level);
}

int Log::getLogLevel() {
  return s_Logger->getLogLevel();
}

void Log::setName(std::string name) {
  s_Logger = std::make_shared<Logger>(name);
}

void Log::print(int level, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  s_Logger->print(level, fmt, args);
  va_end(args);
}

void Log::hexdump(int level, const char *prefix, const void *data, size_t size, size_t len) {
  s_Logger->hexdump(level, prefix, data, size, len);
}

void Log::vprint(const char *fmt, va_list args) {
  s_Logger->vprint(fmt, args);
}

void Log::putc(char chr) {
  s_Logger->putc(chr);
}

extern "C" void std_set_log_name(const char *name) {
  Log::setName(std::string(name));
}

extern "C" int std_get_log_level(void) {
  return Log::getLogLevel();
}

extern "C" void std_set_log_level(int level) {
  Log::setLogLevel(level);
}

extern "C" void __putchar(char chr) {
  Log::putc(chr);
}

#ifndef USE_STDIO_CAN
extern "C" int std_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  (void)Log::vprint(fmt, args);
  va_end(args);
  return 0;
}
#endif
} /* namespace as */
