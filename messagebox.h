#ifndef MESSAGEBOX_X11_MESSAGEBOX_H
#define MESSAGEBOX_X11_MESSAGEBOX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MBOX_BUTTONFLAGS_NONE = 0,
    MBOX_BUTTONFLAGS_RETURNKEY_DEFAULT = (1<<0),
    MBOX_BUTTONFLAGS_ESCAPEKEY_DEFAULT = (1<<1)
} MessageBoxButtonFlags;

typedef struct Button{
    MessageBoxButtonFlags flags;
    int result;
    const char *label;
}Button;

int Messagebox(const char* title, const char* text, const Button* buttons, int numButtons);

#ifdef __cplusplus
}
#endif
    
#endif //MESSAGEBOX_X11_MESSAGEBOX_H
