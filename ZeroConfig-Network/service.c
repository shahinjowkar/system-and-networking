#include <unistd.h>
#include "zcs.h"
#include <stdlib.h>
#include <stdio.h>



int main() {
    int rv;
    rv = zcs_init(ZCS_SERVICE_TYPE);
    zcs_attribute_t attribs[] = {
	    { .attr_name = "type", .value = "speaker"},
	    { .attr_name = "location", .value = "kitchen"},
	    { .attr_name = "make", .value = "yamaha"} };
    printf("before zcs start \n");
    rv = zcs_start("speaker-X", attribs, sizeof(attribs)/sizeof(zcs_attribute_t));
    printf("after zcs start \n");
    for (int i = 0; i < 1000; i++) {
        printf("before post add");
        rv = zcs_post_ad("mute", "on");
        printf("after post add");
        sleep(10);
        rv = zcs_post_ad("mute", "off");
        sleep(10);
    }
    printf("Shutting down\n");
    rv = zcs_shutdown();
}

