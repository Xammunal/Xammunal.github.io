//
//  ChatWindowController.m
//  Курсовая "Чат"
//
//  Created by Иван Салухов on 05.06.2025.
//

#import "ChatWindowController.h"
#import "client.h"

@implementation ChatWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    [self.chatTextView setEditable:NO];
    [self.chatTextView setSelectable:YES];

    client_register_callback(on_server_message);
}

- (IBAction)sendButtonPressed:(id)sender {
    NSString *message = self.inputField.stringValue;
    if (message.length == 0) return;
    client_send_message(message.UTF8String);
    self.inputField.stringValue = @"";
}

void on_server_message(const char* msg) {
    dispatch_async(dispatch_get_main_queue(), ^{
        ChatWindowController *activeController = (ChatWindowController*)NSApp.keyWindow.windowController;
        if (!activeController || ![activeController isKindOfClass:[ChatWindowController class]]) return;

        NSTextView *chatView = activeController.chatTextView;
        NSString *currentText = chatView.string;
        NSString *newLine = [NSString stringWithFormat:@"\n%@", [NSString stringWithUTF8String:msg]];
        NSString *updated = [currentText stringByAppendingString:newLine];

        [chatView setString:updated];
        [chatView scrollRangeToVisible:NSMakeRange(updated.length, 0)];
    });
}

@end
