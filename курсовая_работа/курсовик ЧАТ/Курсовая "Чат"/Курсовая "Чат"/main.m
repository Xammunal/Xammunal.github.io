//
//  main.m
//  Курсовая "Чат"
//
//  Created by Иван Салухов on 29.05.2025.
//

#import <Cocoa/Cocoa.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
    }
    setbuf(stdout, NULL); // отключает буферизацию stdout
    return NSApplicationMain(argc, argv);
}
