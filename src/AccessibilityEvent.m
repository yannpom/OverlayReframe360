#import <Foundation/Foundation.h>
#import <Foundation/NSString.h>
#import <CoreGraphics/CGDirectDisplay.h>
#import <CoreGraphics/CGEventTypes.h>
#import <AppKit/AppKit.h>

#include "AccessibilityEvent.h"

static CFStringRef path[] = {
    kAXWindowRole,
    kAXGroupRole,
    kAXGroupRole,
    kAXGroupRole,
    kAXButtonRole
};

static char * buttonDescription = "Yann Button";

static AXUIElementRef yannButton = NULL;

static void getChildrenRecursive(int depth, AXUIElementRef axObject) {
    if (depth >= 5) {
        return;
    }
    NSLog(@"depth=%d", depth);
    CFStringRef childrenRole = path[depth];
    
    AXError axErr;
    CFTypeRef typeRef;

    CFIndex objectChildrenCount;
    axErr = AXUIElementGetAttributeValueCount(axObject, kAXChildrenAttribute, &objectChildrenCount);
    assert(kAXErrorSuccess == axErr);
    CFArrayRef objectChildren;
    axErr = AXUIElementCopyAttributeValues(axObject, kAXChildrenAttribute, 0, objectChildrenCount, &objectChildren);
    assert(kAXErrorSuccess == axErr);
    assert(CFArrayGetCount(objectChildren) == objectChildrenCount);

    for (CFIndex i = 0; i < objectChildrenCount; ++i) {
        AXUIElementRef objectChild = CFArrayGetValueAtIndex(objectChildren, i);
        axErr = AXUIElementCopyAttributeValue(objectChild, kAXRoleAttribute, &typeRef);
        assert(kAXErrorSuccess == axErr);
        assert([(__bridge id)typeRef isKindOfClass:[NSString class]]);
        CFStringRef role = (CFStringRef)typeRef;
        NSLog(@"role=%@", role);
        if (kCFCompareEqualTo == CFStringCompare(role, childrenRole, 0)) {
            getChildrenRecursive(depth+1, objectChild);
            if (depth == 4) {
                axErr = AXUIElementCopyAttributeValue(objectChild, kAXDescriptionAttribute, &typeRef);
                assert(kAXErrorSuccess == axErr);
                assert([(__bridge id)typeRef isKindOfClass:[NSString class]]);
                CFStringRef description = (CFStringRef)typeRef;
                NSLog(@"description %@", description);
                const char * c_description =  [(__bridge NSString *)description UTF8String];
                if (strcmp(c_description, buttonDescription) == 0) {
                    NSLog(@"FOUND!!!!!!!!!!!");
                    yannButton = objectChild;
                }
            }
            //i = objectChildrenCount - 1;
        }
        //CFRelease(role);
    }
    //CFRelease(objectChildren);
}

int get_pid_by_bundle_identifier(const char * bundle_id) {
    NSArray<NSRunningApplication *> * ret = [NSRunningApplication runningApplicationsWithBundleIdentifier:[NSString stringWithUTF8String:bundle_id]];
    if ([ret count] == 0) {
        return 0;
    }
    NSRunningApplication * o = [ret objectAtIndex:0];
    return o.processIdentifier;
}

void press_button() {
    const int resolve_pid = get_pid_by_bundle_identifier("com.blackmagic-design.DaVinciResolve");
    assert(resolve_pid);

    printf("press_button, got resolve_pid: %d\n", resolve_pid);
    AXUIElementRef resolve = AXUIElementCreateApplication(resolve_pid);
    getChildrenRecursive(0, resolve);
    printf("press_button yannButton: %p\n", yannButton);
    int ret = AXUIElementPerformAction(yannButton, kAXPressAction);
    printf("press_button ret: %d\n", ret);
}