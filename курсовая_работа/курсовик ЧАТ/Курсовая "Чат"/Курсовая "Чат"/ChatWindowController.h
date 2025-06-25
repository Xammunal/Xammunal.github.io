//
//  ChatWindowController.h
//  Курсовая "Чат"
//
//  Created by Иван Салухов on 05.06.2025.
//

#import <Cocoa/Cocoa.h>

@interface ChatWindowController : NSWindowController

@property (unsafe_unretained) IBOutlet NSTextView *chatTextView;
@property (weak) IBOutlet NSTextField *inputField;
@property (weak) IBOutlet NSButton *sendButton;

- (IBAction)sendButtonPressed:(id)sender;

@end
