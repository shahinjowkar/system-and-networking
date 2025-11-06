#ifndef __ZCS_H__
#define __ZCS_H__

#define ZCS_APP_TYPE                1
#define ZCS_SERVICE_TYPE            2

#include <time.h>

typedef struct {
    char *attr_name;
    char *value;
} zcs_attribute_t;

typedef struct entry {
    char *name;
    int status;
    time_t latestTimestamp;
    zcs_attribute_t *attributes;
    size_t numAttr;
    struct entry * next;
} registry_entry_t;


// //what data do we have for the log?
// typedef struct log {
//     char *name;
//     //seperates each down and up with "#"
//     char *history
//     int prevHeartBeat;
//     int currHeartBeat;
//     size_t numAttr;
//     struct entry * next;
// } node_log;




typedef void (*zcs_cb_f)(char *, char *);

int zcs_init(int type);
// int zcs_start(char *name, zcs_attribute_t attr[], int num);
// int zcs_post_ad(char *ad_name, char *ad_value);
// int zcs_query(char *attr_name, char *attr_value, char *node_names[], int namelen);
// int zcs_get_attribs(char *name, zcs_attribute_t attr[], int *num);
// int zcs_listen_ad(char *name, zcs_cb_f cback);
// int zcs_shutdown();
// void zcs_log();
void *receive();
void *testInit();

#endif

