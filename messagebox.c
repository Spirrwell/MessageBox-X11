/*
 * Copyright (c) 2018 Eleobert Espírito Santo eleobert@hotmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <locale.h>
#include <wchar.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "messagebox.h"


struct Dimensions{
    //window
    unsigned int winMinWidth;
    unsigned int winMinHeight;
    //vertical space between lines
    unsigned int lineSpacing;
    unsigned int barHeight;
    //padding
    unsigned int pad_up;
    unsigned int pad_down;
    unsigned int pad_left;
    unsigned int pad_right;
    //button
    unsigned int btSpacing;
    unsigned int btMinWidth;
    unsigned int btMinHeight;
    unsigned int btLateralPad;

};


typedef struct ButtonData{ //<----see static
    const Button* button;
    GC *gc;
    XRectangle rect;
}ButtonData;

//these values can be changed to whatever you prefer
struct Dimensions dim = {400, 0, 5, 50, 30, 10, 30, 30, 20, 75, 25, 8};


void setWindowTitle(const char* title, const Window *win, Display *dpy){
    Atom wm_Name = XInternAtom(dpy, "_NET_WM_NAME", False);
    Atom utf8Str = XInternAtom(dpy, "UTF8_STRING", False);

    Atom winType = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False );
    Atom typeDialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False );

    XChangeProperty(dpy, *win, wm_Name, utf8Str, 8, PropModeReplace, (const unsigned char*)title, (int)strlen(title));

    XChangeProperty( dpy,*win, winType, XA_ATOM,
                     32, PropModeReplace, (unsigned char *)&typeDialog,
                     1);
}

void split(const char* text, const char* seps, char ***str, int *count){
    char *tok, *data;
    int i;
    *count = 0;
    data = strdup(text);

    for (tok = strtok(data, seps); tok != NULL; tok = strtok(NULL, seps))
        (*count)++;

    free(data);
    fflush(stdout);
    data = strdup(text);
    *str = (char **)malloc((size_t)(*count)*sizeof(char*));


    for (i = 0, tok = strtok(data, seps); tok != NULL; tok = strtok(NULL, seps), i++)
        (*str)[i] = strdup(tok);
    free(data);
}

void computeTextSize(XFontSet* fs, char** texts, int size, unsigned int spaceBetweenLines,
                     unsigned int *w,  unsigned  int *h)
{
    int i;
    XRectangle rect  = {0,0,0,0};
    *h = 0;
    *w = 0;
    for(i = 0; i < size; i++){
        Xutf8TextExtents(*fs, texts[i], (int)strlen(texts[i]), &rect, NULL);
        *w = (rect.width > *w) ? (rect.width): *w;
        *h += rect.height + spaceBetweenLines;
        fflush(stdin);
    }
}

void createColor(const Colormap* cmap, Display* dpy, XColor* color,
                unsigned char red, unsigned char green, unsigned char blue)
{
    const float coloratio = (float) 65535 / 255;
    memset(color, 0, sizeof(XColor));
    color->red   = (unsigned short)(coloratio * red  );
    color->green = (unsigned short)(coloratio * green);
    color->blue  = (unsigned short)(coloratio * blue );

    XAllocColor (dpy, *cmap, color);
}

void createGC(GC* gc, const Colormap* cmap, Display* dpy, const  Drawable* drawable,
              unsigned char red, unsigned char green, unsigned char blue){
    float coloratio = (float) 65535 / 255;
    XColor color;
    *gc = XCreateGC (dpy, *drawable, 0,0);
    memset(&color, 0, sizeof(color));
    color.red   = (unsigned short)(coloratio * red  );
    color.green = (unsigned short)(coloratio * green);
    color.blue  = (unsigned short)(coloratio * blue );
    color.flags = DoRed | DoGreen | DoBlue;
    XAllocColor (dpy, *cmap, &color);
    XSetForeground (dpy, *gc, color.pixel);
}

void createTextGC(GC* gc, const Colormap* cmap, Display* dpy, const  Window* win, XGCValues *gcValues,
              unsigned char red, unsigned char green, unsigned char blue){
    float coloratio = (float) 65535 / 255;
    XColor color;
    *gc = XCreateGC (dpy, *win, GCFont + GCForeground, gcValues);
    memset(&color, 0, sizeof(color));
    color.red   = (unsigned short)(coloratio * red  );
    color.green = (unsigned short)(coloratio * green);
    color.blue  = (unsigned short)(coloratio * blue );
    color.flags = DoRed | DoGreen | DoBlue;
    XAllocColor (dpy, *cmap, &color);
    XSetForeground (dpy, *gc, color.pixel);
}

bool isInside(int x, int y, XRectangle rect){
    if(x < rect.x || x > (rect.x + rect.width) || y < rect.y || y > (rect.y + rect.height))
        return false;
    return true;
}

int Messagebox(const char* title, const char* text, const Button* buttons, int numButtons)
{
    char* oldLocale = strdup(setlocale(LC_ALL, NULL));
    setlocale(LC_ALL,"");

    //---- convert the text in list (to draw in multiply lines)---------------------------------------------------------
    char** text_splitted = NULL;
    int textLines = 0;
    split(text, "\n" , &text_splitted, &textLines);
    //------------------------------------------------------------------------------------------------------------------

    Display* dpy = NULL;
    dpy = XOpenDisplay(0);
    if(dpy == NULL){
        fprintf(stderr, "Error opening display display.");
    }

    int ds = DefaultScreen(dpy);
    Colormap cmap = DefaultColormap (dpy, ds);

    XColor backgroundColor;
    createColor(&cmap, dpy, &backgroundColor, 69, 69, 69);

    Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, ds), 0, 0, 800, 100, 1,
                                     BlackPixel(dpy, ds), backgroundColor.pixel);


    XSelectInput(dpy, win, ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask);
    XMapWindow(dpy, win);

    //allow windows to be closed by pressing cross button
    Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &WM_DELETE_WINDOW, 1);

    //--create the gc for drawing text----------------------------------------------------------------------------------
    XGCValues gcValues;
    gcValues.font = XLoadFont(dpy, "fixed");
    gcValues.foreground = BlackPixel(dpy, 0);
    GC textGC;
    createTextGC(&textGC, &cmap, dpy, &win, &gcValues, 218, 218, 218);
    XUnmapWindow( dpy, win );
    //------------------------------------------------------------------------------------------------------------------

    //----create fontset-----------------------------------------------------------------------------------------------
    char **missingCharset_list = NULL;
    int missingCharset_count = 0;
    XFontSet fs;
    fs = XCreateFontSet(dpy,
                        "fixed",
                        &missingCharset_list, &missingCharset_count, NULL);

    if (missingCharset_count) {
        fprintf(stderr, "Missing charsets :\n");
        for(int i = 0; i < missingCharset_count; i++){
            fprintf(stderr, "%s\n", missingCharset_list[i]);
        }
        XFreeStringList(missingCharset_list);
        missingCharset_list = NULL;
    }
    //------------------------------------------------------------------------------------------------------------------

    //resize the window according to the text size-----------------------------------------------------------------------
    unsigned int winW, winH;
    unsigned int textW, textH;

    //calculate the ideal window's size
    computeTextSize(&fs, text_splitted, textLines, dim.lineSpacing, &textW, &textH);
    unsigned int newWidth = textW + dim.pad_left + dim.pad_right;
    unsigned int newHeight = textH + dim.pad_up + dim.pad_down + dim.barHeight;
    winW = (newWidth > dim.winMinWidth)? newWidth: dim.winMinWidth;
    winH = (newHeight > dim.winMinHeight)? newHeight: dim.winMinHeight;

    //------------------------------------------------------------------------------------------------------------------

    GC barGC;
    GC buttonGC;
    GC buttonGC_underPointer;
    GC buttonGC_onClick;
    GC buttonGC_outline;                               // GC colors
    createGC(&barGC, &cmap, dpy, &win,                 37, 37, 37);
    createGC(&buttonGC, &cmap, dpy, &win,              69, 69, 69);
    createGC(&buttonGC_underPointer, &cmap, dpy, &win, 59, 59, 59);
    createGC(&buttonGC_onClick, &cmap, dpy, &win,      49, 49, 49);
    createGC(&buttonGC_outline, &cmap, dpy, &win,      122, 122, 122);

    //---setup the buttons data-----------------------------------------------------------------------------------------
    ButtonData *btsData;
    btsData = (ButtonData*)malloc((size_t)numButtons * sizeof(ButtonData));

    int padDiff = 0;
    int pass = 0;
    for(int i = 0; i < numButtons; i++){
        btsData[i].button = &buttons[i];
        btsData[i].gc = &buttonGC;
        XRectangle btTextDim;
        Xutf8TextExtents(fs, btsData[i].button->label, (int)strlen(btsData[i].button->label),
                       &btTextDim, NULL);
        btsData[i].rect.width = (btTextDim.width < dim.btMinWidth) ? dim.btMinWidth:
                                   (btTextDim.width + 2 * dim.btLateralPad);
        btsData[i].rect.height = dim.btMinHeight;
        btsData[i].rect.x = winW - dim.pad_left - btsData[i].rect.width - pass;
        btsData[i].rect.y = textH + dim.pad_up + dim.pad_down + ((dim.barHeight - dim.btMinHeight)/ 2) ;
        
        if (btsData[i].rect.x < (int)dim.pad_left)
            padDiff = dim.pad_left - btsData[i].rect.x;

        pass += btsData[i].rect.width + dim.btSpacing;
    }

    // Our buttons have exceeded our window dimensions
    if (padDiff > 0)
    {
        pass = 0;
        winW += padDiff;

        for (int i = 0; i < numButtons; ++i)
        {
            btsData[i].rect.x = winW - dim.pad_left - btsData[i].rect.width - pass;
            pass += btsData[i].rect.width + dim.btSpacing;
        }
    }

    // setup a back buffer that we copy from
    XWindowAttributes wa;
    XGetWindowAttributes(dpy, win, &wa);

    Pixmap backBuffer = XCreatePixmap(dpy, win, winW, winH, wa.depth);

    GC backBufferGC;
    createGC(&backBufferGC, &cmap, dpy, &backBuffer, 69, 69, 69);

    XFillRectangle(dpy, backBuffer, backBufferGC, 0, 0, winW, winH);

    //set windows hints
    XSizeHints hints;
    hints.flags      = PSize | PMinSize | PMaxSize;
    hints.min_width  = hints.max_width  = hints.base_width  = winW;
    hints.min_height = hints.max_height = hints.base_height = winH;

    XSetWMNormalHints( dpy, win, &hints );
    XMapRaised( dpy, win );

    setWindowTitle(title, &win, dpy);

    XFlush( dpy );

    bool quit = false;
    int res = -1;
    int pressedIndex = -1;

    while( !quit ) {
        XEvent e;
        XNextEvent( dpy, &e );

        switch (e.type){

            case MotionNotify:
            case ButtonPress:
            case ButtonRelease:
            case Expose:
                if (e.type != Expose)
                {
                    for(int i = 0; i < numButtons; i++) {
                        btsData[i].gc = &buttonGC;
                        if (isInside(e.xmotion.x, e.xmotion.y, btsData[i].rect)) {
                            btsData[i].gc = (pressedIndex == i) ? &buttonGC_onClick : &buttonGC_underPointer;
                            if(e.type == ButtonPress && e.xbutton.button == Button1) {
                                btsData[i].gc = &buttonGC_onClick;
                                pressedIndex = i;
                            }
                            else if (e.type == ButtonRelease && e.xbutton.button == Button1)
                            {
                                if (pressedIndex == i)
                                {
                                    res = btsData[i].button->result;
                                    quit = true;
                                }
                            }
                        }
                    }

                    if (e.type == ButtonRelease && e.xbutton.button == Button1)
                        pressedIndex = -1;

                    XEvent exp;
                    memset(&exp, 0, sizeof(exp));

                    exp.type = Expose;
                    exp.xexpose.window = win;
                    XSendEvent(dpy, win, False, ExposureMask, &exp);
                }
                else
                {
                    //draw the text in multiply lines----------------------------------------------------------------------
                    for(int i = 0; i < textLines; i++){
                        Xutf8DrawString( dpy, backBuffer, fs, textGC, dim.pad_left, dim.pad_up + i * (dim.lineSpacing + 18),
                                    text_splitted[i], (int)strlen(text_splitted[i]));
                    }
                    //------------------------------------------------------------------------------------------------------
                    XFillRectangle(dpy, backBuffer, barGC, 0, textH + dim.pad_up + dim.pad_down, winW, dim.barHeight);

                    for(int i = 0; i < numButtons; i++){
                        XFillRectangle(dpy, backBuffer, *btsData[i].gc, btsData[i].rect.x, btsData[i].rect.y,
                                    btsData[i].rect.width, btsData[i].rect.height);
                        XDrawRectangle(dpy, backBuffer, buttonGC_outline, btsData[i].rect.x, btsData[i].rect.y,
                                        btsData[i].rect.width, btsData[i].rect.height);

                        XRectangle btTextDim;
                        Xutf8TextExtents(fs, btsData[i].button->label, (int)strlen(btsData[i].button->label),
                                    &btTextDim, NULL);
                        Xutf8DrawString( dpy, backBuffer, fs, textGC,
                                    btsData[i].rect.x + (btsData[i].rect.width  - btTextDim.width ) / 2,
                                    btsData[i].rect.y + (btsData[i].rect.height + btTextDim.height) / 2,
                                    btsData[i].button->label, (int)strlen(btsData[i].button->label));
                    }

                    // copy from the back buffer so that nothing flickers
                    XCopyArea(dpy, backBuffer, win, backBufferGC, 0, 0, winW, winH, 0, 0);
                }

                break;
            
            case KeyPress:
                const KeySym sym = XLookupKeysym(&e.xkey, 0);
                for (int i = 0; i < numButtons; ++i)
                {
                    if (sym == XK_Escape && (btsData[i].button->flags & MBOX_BUTTONFLAGS_ESCAPEKEY_DEFAULT))
                    {
                        res = btsData[i].button->result;
                        quit = true;
                        break;
                    }
                    else if (sym == XK_Return && (btsData[i].button->flags & MBOX_BUTTONFLAGS_RETURNKEY_DEFAULT))
                    {
                        res = btsData[i].button->result;
                        quit = true;
                        break;
                    }
                }
                break;

            case ClientMessage:
                // if window's cross button pressed, quit
                if((unsigned int)(e.xclient.data.l[0]) == WM_DELETE_WINDOW)
                    quit = true;
                break;
            default:
                break;
        }
    }

    for(int i = 0; i < textLines; i++){
        free(text_splitted[i]);
    }

    setlocale(LC_ALL, oldLocale);

    free(text_splitted);
    free(btsData);
    free(oldLocale);
    if (missingCharset_list)
        XFreeStringList(missingCharset_list);
    XDestroyWindow(dpy, win);
    XFreeFontSet(dpy, fs);
    XFreeGC(dpy, textGC);
    XFreeGC(dpy, barGC);
    XFreeGC(dpy, buttonGC);
    XFreeGC(dpy, buttonGC_underPointer);
    XFreeGC(dpy, buttonGC_onClick);
    XFreeGC(dpy, buttonGC_outline);
    XFreeGC(dpy, backBufferGC);
    XFreeColormap(dpy, cmap);
    XCloseDisplay(dpy);

    return res;
}
