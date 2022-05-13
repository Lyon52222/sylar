#include "iostream"
#include "lyon/hook.h"
#include "lyon/iomanager.h"
#include "unistd.h"
#include <lyon/log.h>

static lyon::Logger::ptr g_logger = LYON_LOG_GET_ROOT();

void test_sleep() {
    lyon::IOManager iom(1, true, "hook_test");
    iom.start();
    iom.addJob([]() {
        sleep(2);
        LYON_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.addJob([]() {
        sleep(3);
        LYON_LOG_INFO(g_logger) << "sleep 3";
    });
    LYON_LOG_INFO(g_logger) << "test_sleep";

    iom.stop();
}

int main(int argc, char *argv[]) {
    lyon::set_hook_enable(true);
    test_sleep();
    return 0;
}