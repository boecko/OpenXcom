//
//  HappyMain.m
//  Demos
//
//  Created by Andreas Böckler on 15.09.14.
//
//

#include "SDL.h"
#include "SDLMain.h"

#ifdef IOS_SDLMAIN
#ifdef main
#  undef main
#endif

int main(int argc, char *argv[]) {
    int retVal = 0;
    @autoreleasepool {
        @try {
            retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([SDLMain class]));
        }
        @catch (NSException *exception) {
            NSLog(@"Exception - %@",[exception description]);
            NSLog(@"%@", [exception callStackSymbols]);
            exit(EXIT_FAILURE);
        }
    }
    return retVal;
}

@implementation SDLMain
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    int status;
    
    /* Set the working directory to the .app's parent directory */
        //[self setupWorkingDirectory:gFinderLaunch];
    
    /* Hand off to main application code */
    status = SDL_main (NULL, NULL);
    
    /* We're done, thank you for playing */
    exit(status);
}
@end
#endif