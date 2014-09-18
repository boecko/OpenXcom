//
// SDLMain.mm
//
//  Created by Andreas Böckler on 15.09.14.
//
//

#include "SDL.h"
#include "SDLMain.h"

#define IOS_SDLMAIN 1

#ifdef IOS_SDLMAIN
#ifdef main
#  undef main
#endif

@implementation SDLUIKitDelegate (customDelegate)
+(NSString *)getAppDelegateClassName {
    return @"SDLMain";
}
@end

extern UIWindow *launch_window;

@implementation SDLMain

- (void)postFinishLaunch
{
    
    int width = [[UIScreen mainScreen] bounds].size.width;
    int height = [[UIScreen mainScreen] bounds].size.height;
    NSMutableArray *cmdLineArgs = [[NSMutableArray alloc] init];
//    [cmdLineArgs addObject:@"SDLApp"];
//    [cmdLineArgs addObject:@"-displayWidth"];
//    [cmdLineArgs addObject:[NSString stringWithFormat:@"%i",width]];
//    [cmdLineArgs addObject:@"-displayHeight"];
//    [cmdLineArgs addObject:[NSString stringWithFormat:@"%i",height]];
    
    unsigned argc = [cmdLineArgs count];
    char **argv = (char **)malloc((argc+1) * sizeof(char*));
    for (unsigned i = 0; i < argc; i++)
    {
        argv[i] = strdup([[cmdLineArgs objectAtIndex:i] UTF8String]);
    }
    argv[argc] = NULL;
    
    /* run the user's application, passing argc and argv */
    SDL_iPhoneSetEventPump(SDL_TRUE);
    int exit_status = SDL_main(argc, argv);
    SDL_iPhoneSetEventPump(SDL_FALSE);
    if (argv != NULL)
    {
        for (unsigned index = 0; argv[index] != NULL; index++)
        {
            free(argv[index]);
        }
        free(argv);
    }
//    /* If we showed a splash image, clean it up */
//    if (launch_window) {
//        [launch_window release];
//        launch_window = NULL;
//    }
    
    /* exit, passing the return status from the user's application */
    /* We don't actually exit to support applications that do setup in
     * their main function and then allow the Cocoa event loop to run.
     */
    /* exit(exit_status); */
}

@end
#endif