/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#ifndef _SDLMain_h_
#define _SDLMain_h_

#import <UIKit/UIKit.h>
#include "../SDL/src/video/uikit/SDL_uikitappdelegate.h"

@interface SDLUIKitDelegate (customDelegate)
@end

@interface SDLMain : SDLUIKitDelegate

@end

#endif /* _SDLMain_h_ */
