//
//  LoginViewController.h
//  Курсовая "Чат"
//
//  Created by Иван Салухов on 29.05.2025.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface LoginViewController : NSViewController

@property (weak) IBOutlet NSTextField *nameField;
@property (weak) IBOutlet NSSecureTextField *keyField;
@property (strong) NSWindowController *chatWindowController;

@end

NS_ASSUME_NONNULL_END
