#ifndef SETDESKTOP_H
#define SETDESKTOP_H
// Attempt to identify a window by name or attribute.
// by Adam Pierce <adam@doctort.org>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <iostream>
#include <list>
#include <stdlib.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>



using namespace std;

class WindowsMatchingPid
{
public:
    WindowsMatchingPid(Display *display, Window wRoot, unsigned long pid)
        : _display(display)
        , _pid(pid)
    {
        // Get the PID property atom.
        _atomPID = XInternAtom(display, "_NET_WM_PID", True);
        if (_atomPID == None) {
            cout << "No such atom" << endl;
            return;
        }

        search(wRoot);
    }

    const list<Window> &result() const { return _result; }
    const list<Window> &allresult() const { return _allresult; }
private:
    unsigned long  _pid;
    Atom           _atomPID;
    Display       *_display;
    list<Window>   _result;
   list<Window>   _allresult;
//    extern int FULLSCREENXGetWindowProperty(
//        Display*		/* display */,
//        Window		/* w */,
//        Atom		/* property */,
//        long		/* long_offset */,
//        long		/* long_length */,
//        Bool		/* delete */,
//        Atom		/* req_type */,
//        Atom*		/* actual_type_return */,
//        int*		/* actual_format_return */,
//        unsigned long*	/* nitems_return */,
//        unsigned long*	/* bytes_after_return */,
//        unsigned char**	/* prop_return */
//    );
    void search(Window w)
    {
        // Get the PID for the current Window.
        Atom           type;
        int            format;
        unsigned long  nItems;
        unsigned long  bytesAfter;
        unsigned char *propPID = 0;
//        int xxx= FULLSCREENXGetWindowProperty(_display, w, _atomPID, 0, 1, False, XA_CARDINAL,
//                                              &type, &format, &nItems, &bytesAfter, &propPID);
//        {
//            if (propPID != 0) {
//                std::cout<<propPID;
//            }
//        }
        if (Success == XGetWindowProperty(_display, w, _atomPID, 0, 1, False, XA_CARDINAL,
                                          &type, &format, &nItems, &bytesAfter, &propPID)) {
            if (propPID != 0) {
                _allresult.push_back(w);
                // If the PID matches, add this window to the result set.
                if (_pid == *((unsigned long *)propPID))
                    _result.push_back(w);
                if((unsigned long *)propPID!=0){
                    unsigned long xx= *(unsigned long *)propPID;
                    std::cout<<xx;
                }

                XFree(propPID);
            }
        }

        // Recurse into child windows.
        Window    wRoot;
        Window    wParent;
        Window   *wChild;
        unsigned  nChildren;
        if (0 != XQueryTree(_display, w, &wRoot, &wParent, &wChild, &nChildren)) {
            for (unsigned i = 0; i < nChildren; i++)
                search(wChild[i]);
        }
    }
};
#endif // SETDESKTOP_H
