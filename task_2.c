#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t t;
    struct tm *tm;

    char *time_zone_str = getenv("TZ");
    printf("actual time zone %s \n", time_zone_str);

    time(&t);
    tm = localtime(&t);
    printf("befor %s \n", asctime(tm));

    setenv("TZ", "America/Los_Angeles", 1);
    tzset();

    time(&t);
    tm = localtime(&t);

    printf("after %s", asctime(tm));

    return 0;
}