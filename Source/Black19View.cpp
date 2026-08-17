#include "Black19Processor.h"

#if defined (_WIN32)

#include "public.sdk/source/common/pluginview.h"
#include <windows.h>
#include <cstdio>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace bbk {

static const wchar_t* kWndClassName = L"BBKBlack19ViewWnd";
static const int kViewWidth = 380;
static const int kViewHeight = 220;

class Black19View : public CPluginView
{
public:
    explicit Black19View (Black19Component* p) : processor (p)
{
        rect.left = 0; rect.top = 0; rect.right = kViewWidth; rect.bottom = kViewHeight;
}

    tresult PLUGIN_API isPlatformTypeSupported (FIDString type) SMTG_OVERRIDE
{
          return (strcmp (type, "HWND") == 0) ? kResultTrue : kResultFalse;
}

    tresult PLUGIN_API attached (void* parent, FIDString type) SMTG_OVERRIDE
{
          if (strcmp (type, "HWND") != 0) return kResultFalse;

        static bool classRegistered = false;
        if (! classRegistered)
{
            WNDCLASSW wc {};
            wc.lpfnWndProc = &Black19View::wndProcStatic;
            wc.hInstance = GetModuleHandleW (nullptr);
            wc.lpszClassName = kWndClassName;
            wc.hbrBackground = CreateSolidBrush (RGB (0x17, 0x17, 0x17));
            wc.hCursor = LoadCursorW (nullptr, (LPCWSTR) IDC_ARROW);
            RegisterClassW (&wc);
            classRegistered = true;
}

        hwnd = CreateWindowExW (0, kWndClassName, L"BBK Black-19",
                                           WS_CHILD | WS_VISIBLE,
                                           0, 0, kViewWidth, kViewHeight,
                                           (HWND) parent, nullptr, GetModuleHandleW (nullptr), this);
        systemWindow = parent;

        buildChildControls();
        SetTimer (hwnd, 1, 250, nullptr);
        refresh();
        return kResultTrue;
}

    tresult PLUGIN_API removed () SMTG_OVERRIDE
{
          if (hwnd) { KillTimer (hwnd, 1); DestroyWindow (hwnd); hwnd = nullptr; }
        systemWindow = nullptr;
        return kResultTrue;
}

private:
    Black19Component* processor;
    HWND hwnd = nullptr;
    HWND lblTitle = nullptr, lblRate = nullptr, lblStatus = nullptr, btnToggle = nullptr, lblSpec = nullptr, lblDiag = nullptr, lblTiming = nullptr;

    void buildChildControls()
{
          HFONT titleFont = CreateFontW (22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH, L"Segoe UI");
        HFONT normalFont = CreateFontW (15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                         DEFAULT_PITCH, L"Segoe UI");

        lblTitle = CreateWindowExW (0, L"STATIC", L"BBK Black-19", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                               10, 10, kViewWidth - 20, 30, hwnd, nullptr, GetModuleHandleW (nullptr), nullptr);
        SendMessageW (lblTitle, WM_SETFONT, (WPARAM) titleFont, TRUE);

        lblRate = CreateWindowExW (0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                              10, 46, kViewWidth - 20, 22, hwnd, nullptr, GetModuleHandleW (nullptr), nullptr);
        SendMessageW (lblRate, WM_SETFONT, (WPARAM) normalFont, TRUE);

        lblStatus = CreateWindowExW (0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                                10, 70, kViewWidth - 20, 22, hwnd, nullptr, GetModuleHandleW (nullptr), nullptr);
        SendMessageW (lblStatus, WM_SETFONT, (WPARAM) normalFont, TRUE);

        btnToggle = CreateWindowExW (0, L"BUTTON", L"BLACK-19 ON",
                                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                                (kViewWidth - 140) / 2, 100, 140, 30, hwnd,
                                                (HMENU) 1001, GetModuleHandleW (nullptr), nullptr);
        SendMessageW (btnToggle, WM_SETFONT, (WPARAM) normalFont, TRUE);

        lblSpec = CreateWindowExW (0, L"STATIC",
                                              L"192 kHz only | 19 taps | pass 0-20 kHz | stop 76-96 kHz\n"
                                              L"linear phase | 9-sample delay | minimum peak sidelobe",
                                              WS_CHILD | WS_VISIBLE | SS_CENTER,
                                              10, 138, kViewWidth - 20, 30, hwnd, nullptr, GetModuleHandleW (nullptr), nullptr);
        SendMessageW (lblSpec, WM_SETFONT, (WPARAM) normalFont, TRUE);

        HFONT smallFont = CreateFontW (12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH, L"Consolas");
        lblDiag = CreateWindowExW (0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                              10, 172, kViewWidth - 20, 22, hwnd, nullptr, GetModuleHandleW (nullptr), nullptr);
        SendMessageW (lblDiag, WM_SETFONT, (WPARAM) smallFont, TRUE);

        lblTiming = CreateWindowExW (0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                                10, 194, kViewWidth - 20, 22, hwnd, nullptr, GetModuleHandleW (nullptr), nullptr);
        SendMessageW (lblTiming, WM_SETFONT, (WPARAM) smallFont, TRUE);
}

    void refresh()
{
          wchar_t buf[128];
        double sr = processor->getHostSampleRateForUI();
        swprintf (buf, 128, L"Host sample rate: %.0f Hz", sr);
        SetWindowTextW (lblRate, buf);

        bool valid = processor->isRateValidForUI();
        bool on = processor->isEnabledForUI();

        if (valid)
                      SetWindowTextW (lblStatus, on ? L"ACTIVE - 192 kHz" : L"READY - 192 kHz (bypassed)");
        else
                      SetWindowTextW (lblStatus, L"BYPASSED - requires 192 kHz");

        EnableWindow (btnToggle, valid ? TRUE : FALSE);
        SetWindowTextW (btnToggle, on ? L"BLACK-19 ON" : L"BLACK-19 BYPASS");

        wchar_t diag[160];
        int inCh = processor->getLastInChannelsForUI();
        int outCh = processor->getLastOutChannelsForUI();
        int procCh = processor->getLastProcessedChannelsForUI();
        int blk = processor->getLastBlockSizeForUI();
        int fmt = processor->getLastSymbolicSampleSizeForUI();
        if (inCh < 0)
                      swprintf (diag, 160, L"host has not called process() yet");
        else
                      swprintf (diag, 160, L"host in=%d out=%d proc=%d block=%d fmt=%d-bit",
                                               inCh, outCh, procCh, blk, fmt);
        SetWindowTextW (lblDiag, diag);

        wchar_t timing[160];
        double last = processor->getLastProcessMicrosForUI();
        double maxT = processor->getMaxProcessMicrosForUI();
        double budget = processor->getBudgetMicrosForUI();
        if (last < 0.0)
                      swprintf (timing, 160, L"(no timing data yet)");
        else
                      swprintf (timing, 160, L"process(): %.0f us (max %.0f us) / budget %.0f us",
                                               last, maxT, budget);
        SetWindowTextW (lblTiming, timing);
}

    static LRESULT CALLBACK wndProcStatic (HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
          if (msg == WM_NCCREATE)
{
            CREATESTRUCTW* cs = (CREATESTRUCTW*) lp;
            SetWindowLongPtrW (h, GWLP_USERDATA, (LONG_PTR) cs->lpCreateParams);
}

        Black19View* self = (Black19View*) GetWindowLongPtrW (h, GWLP_USERDATA);

        switch (msg)
{
case WM_TIMER:
                if (self) self->refresh();
                return 0;
case WM_COMMAND:
                if (self && LOWORD (wp) == 1001 && HIWORD (wp) == BN_CLICKED)
{
                    self->processor->setEnabledFromUI (! self->processor->isEnabledForUI());
                    self->refresh();
                    return 0;
}
                break;
case WM_DESTROY:
                return 0;
default: break;
}
        return DefWindowProcW (h, msg, wp, lp);
}
};

IPlugView* PLUGIN_API createBlack19View (Black19Component* processor)
{
      return new Black19View (processor);
}

} // namespace bbk

#else // non-Windows: no native view, host falls back to its generic editor

namespace bbk { class Black19Component; }
namespace Steinberg { class IPlugView; }

namespace bbk {
Steinberg::IPlugView* createBlack19View (Black19Component*) { return nullptr; }
}

#endif
