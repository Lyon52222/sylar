#ifndef __LYON_LOG_H__
#define __LYON_LOG_H__
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
namespace lyon {

class Logger;
//日志级别
class LogLevel {
  public:
    enum Level { UNKNOWN = 0, DEBUG, INFO, WARN, ERROR, FATAL };

    static const char *toString(LogLevel::Level level);
};

class LogEvent {
  public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(const char *file, int32_t line, uint32_t threadId,
             std::string &threadName, uint32_t fiberId, uint64_t time);

    const char *getFile() const { return m_file; };
    int32_t getLine() const { return m_line; };
    uint32_t getThreaId() const { return m_threadId; };
    std::string getThreaName() const { return m_threadName; };
    uint32_t getFiberId() const { return m_fiberId; };
    uint64_t getTime() const { return m_time; };
    uint32_t getElapse() const { return m_elapse; };
    std::string getContent() const { return m_content; };
    std::shared_ptr<Logger> getLogger() const { return m_logger; };
    const LogLevel::Level getLevel() const { return m_level; };

  private:
    const char *m_file = nullptr; //文件名
    int32_t m_line = 0;           //行号
    uint32_t m_threadId = 0;      //线程ID
    std::string m_threadName;     //线程名称
    uint32_t m_fiberId = 0;       //协程ID
    uint64_t m_time = 0;          //时间戳
    uint32_t m_elapse = 0;
    std::string m_content;

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

//日志格式化
/**
 * %m 消息
 * %p 日志等级
 * %r 累计毫秒数
 * %t 当前线程ID
 * %d 时间和日期
 * %f 文件名
 * %l 源码行数
 * %n 换行符
 * %T 制表符
 * %F 协程ID
 * %N 线程名
 */
class LogFormatter {
  public:
    typedef std::shared_ptr<LogFormatter> ptr;

    LogFormatter(const std::string &pattern);
    void init();
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       LogEvent::ptr event);
    std::ostream &format(std::ostream &os, std::shared_ptr<Logger> logger,
                         LogLevel::Level level, LogEvent::ptr event);

    class FormatItem {
      public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem();
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) = 0;
    };

  private:
    std::string m_pattern;
    std::list<FormatItem::ptr> m_items;
    static std::unordered_map<
        std::string,
        std::function<LogFormatter::FormatItem::ptr(const std::string &str)>>
        format_items;
};

//日志输出地
class LogAppender {
  public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;

    void setFormatter(LogFormatter::ptr val) { m_formatter = val; };
    LogFormatter::ptr getFormatter() { return m_formatter; };

  protected:
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
};
//输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
  public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
             LogEvent::ptr event) override;
};

//输出到文件的Appender
class FileLogAppender : public LogAppender {
  public:
    FileLogAppender(const std::string &name);
    typedef std::shared_ptr<FileLogAppender> prt;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
             LogEvent::ptr event) override;

    bool reopen();

  private:
    std::string m_fname;
    std::ofstream m_fstream;
};

//日志输出器
class Logger : public std::enable_shared_from_this<Logger> {
  public:
    typedef std::shared_ptr<Logger> ptr;
    Logger(const std::string &name = "root");

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);

    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    LogLevel::Level getLevel() const { return m_level; };
    void setLevel(LogLevel::Level val) { m_level = val; };

  private:
    std::string m_name;
    LogLevel::Level m_level; //满足该级别的日志才会被输出
    std::list<LogAppender::ptr> m_appenders;
};

} // namespace lyon

#endif
