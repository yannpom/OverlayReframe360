#import <CoreGraphics/CGDirectDisplay.h>
#import <CoreGraphics/CGEvent.h>
#include <Carbon/Carbon.h>
#include <sys/time.h>

#include "utils.h"

void sendShiftKey() {
    printf("sendShiftKey\n");
    CGEventRef event1 = CGEventCreateKeyboardEvent(NULL, kVK_Control, true);
    CGEventRef event2 = CGEventCreateKeyboardEvent(NULL, kVK_Control, false);
    CGEventPost(kCGSessionEventTap, event1);
    CGEventPost(kCGSessionEventTap, event2);
}

uint64_t get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}


// https://developer.apple.com/forums/thread/694233
// #include <unistd.h>
// #include <signal.h>
// #include <sys/resource.h>
// #include <stdio.h>

// void crash() {
//     pid_t pid = getpid();
//     struct rlimit l;
//     int ret = getrlimit(RLIMIT_CORE, &l);

//     printf("getrlimit returned %d\n", ret);
//     printf("rlim_cur = %llu\n", l.rlim_cur);
//     printf("rlim_max = %llu\n", l.rlim_max);
//     l.rlim_cur = l.rlim_max;
//     printf("setrlimit returned %d\n", setrlimit(RLIMIT_CORE, &l));
//     printf("Time to kill myself\n");
//     kill(pid, SIGBUS);
// }