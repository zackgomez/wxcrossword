#include <wx/wx.h>
#include "crossword_frame.hpp"
#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif // __WXMAC__

class MyApp : public wxApp
{
public:
    virtual bool OnInit()
    {
#ifdef __WXMAC__
        /* Perhaps set up bringing it to the forefront */
        ProcessSerialNumber PSN;
        GetCurrentProcess(&PSN);
        TransformProcessType(&PSN,kProcessTransformToForegroundApplication);
#endif // __WXMAC__
        crossword_frame* frame = new crossword_frame();
        frame->Show(true);
        frame->SetFocus();

        return true;
    }
};

IMPLEMENT_APP(MyApp)
