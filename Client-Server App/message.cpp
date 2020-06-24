#include "message.h"
#include <string.h>
using namespace std;

string simplify_msg (string ip_addr, int port_no, udp_msg msg) {
    char *helper = (char *) calloc (0, 10);
    sprintf(helper, "%d", port_no);
    string simple_msg = ip_addr + ":" + string(helper) + " - ";
    
    simple_msg += string(msg.topic) + " - ";
    
    switch(msg.data_type) {
        case 0 : {
            simple_msg += "INT - ";
            if (msg.content[0] == 1)
                simple_msg += "-";
            uint32_t *integer = (uint32_t*) (msg.content + 1);
            *integer = ntohl (*integer);
            sprintf(helper, "%d", *integer);
            simple_msg += string(helper) + "\n";
            break;
        }
        case 1 : {

            uint16_t *integer = (uint16_t*) (msg.content);
            *integer = ntohs (*integer);
            float fl;
            fl = 1.0 * (*integer) / 100 ;
            sprintf(helper, "%f", fl);
            simple_msg += "SHORT_REAL - " + string(helper) + "\n"; 
            break;
        }
        case 2 : {
            simple_msg += "FLOAT - ";

            if (msg.content[0] == 1)
                simple_msg += "-";
            
            uint32_t *integer = (uint32_t*) (msg.content + 1);
            *integer = ntohl (*integer);
            float fl;
            fl = 1.0 * (*integer) * pow (10, - msg.content[5]);
            sprintf(helper, "%f", fl);
            
            simple_msg += string(helper) + "\n";
            break;
        }
        case 3 :
            simple_msg += "STRING - " + string(msg.content) + "\n";
            break;
    }

    return simple_msg;
}