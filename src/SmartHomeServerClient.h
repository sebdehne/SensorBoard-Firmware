#ifndef _SMARTHOME_CLIENT_H
#define _SMARTHOME_CLIENT_H

#include "RN2483.h"

class SmartHomeServerClientClass
{
private:
    
public:
    SmartHomeServerClientClass();

    bool ping();
};

extern SmartHomeServerClientClass SmartHomeServerClient;


#endif