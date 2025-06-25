#import "LoginViewController.h"
#import "client.h"
#import "ChatWindowController.h"
#import <Cocoa/Cocoa.h>

@interface LoginViewController ()
@end

@implementation LoginViewController

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (IBAction)LoginButtonPressed:(id)sender {
    [self.view.window makeFirstResponder:nil];

    NSString *name = self.nameField.stringValue;
    NSString *key = self.keyField.stringValue;

    NSLog(@"[DEBUG] Введено имя: %@", name);
    NSLog(@"[DEBUG] Введён ключ: %@", key);

    if (name.length == 0 || key.length == 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Введите имя и ключ!";
        [alert runModal];
        return;
    }

    client_set_credentials(name.UTF8String, key.UTF8String);
    client_register_callback(NULL);

    if (!client_start()) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Не удалось подключиться к серверу.";
        [alert runModal];
        return;
    }

    NSLog(@"Успешное подключение. Переход к чату...");

    NSStoryboard *storyboard = [NSStoryboard storyboardWithName:@"Main" bundle:nil];
    self.chatWindowController = [storyboard instantiateControllerWithIdentifier:@"ChatWindowController"];
    [self.chatWindowController showWindow:self];
}

@end
