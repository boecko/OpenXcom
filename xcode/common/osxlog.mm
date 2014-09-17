//
//  osxlog.mm
//  OpenXcom
//
//  Created by Andreas Böckler on 17.09.14.
//  Copyright (c) 2014 -. All rights reserved.
//

#include "osxlog.h"

void OSXLog(const char *message)
{
    NSLog(@"%s", message);
}