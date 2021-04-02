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

int find_pid_by_name(char *ProcName, int *foundpid)
{
    DIR             *dir;
    struct dirent   *d;
    int             pid, i;
    char            *s;
    int pnlen;

    i = 0;
    foundpid[0] = 0;
    pnlen = strlen(ProcName);

    /* Open the /proc directory. */
    dir = opendir("/proc");
    if (!dir) {
        printf("cannot open /proc");
        return -1;
    }

    /* Walk through the directory. */
    while ((d = readdir(dir)) != NULL) {

        char exe [PATH_MAX + 1];
        char path[PATH_MAX + 1];
        int len;
        int namelen;

        /* See if this is a process */
        if ((pid = atoi(d->d_name)) == 0)       continue;

        snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
        if ((len = readlink(exe, path, PATH_MAX)) < 0)
            continue;
        path[len] = '\0';

        /* Find ProcName */
        s = strrchr(path, '/');
        if (s == NULL) continue;
        s++;

        /* we don't need small name len */
        namelen = strlen(s);
        if (namelen < pnlen)     continue;

        if (!strncmp(ProcName, s, pnlen)) {
            /* to avoid subname like search proc tao but proc taolinke matched */
            if (s[pnlen] == ' ' || s[pnlen] == '\0') {
                foundpid[i] = pid;
                i++;
            }
        }
    }

    foundpid[i] = 0;
    closedir(dir);

    return  0;

}


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

private:
    unsigned long  _pid;
    Atom           _atomPID;
    Display       *_display;
    list<Window>   _result;

    void search(Window w)
    {
        // Get the PID for the current Window.
        Atom           type;
        int            format;
        unsigned long  nItems;
        unsigned long  bytesAfter;
        unsigned char *propPID = 0;
        if (Success == XGetWindowProperty(_display, w, _atomPID, 0, 1, False, XA_CARDINAL,
                                          &type, &format, &nItems, &bytesAfter, &propPID)) {
            if (propPID != 0) {
                // If the PID matches, add this window to the result set.
                if (_pid == *((unsigned long *)propPID))
                    _result.push_back(w);

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
