#define UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>
#include <shellapi.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef far
#undef near

typedef signed __int8  i8;
typedef signed __int16 i16;
typedef signed __int32 i32;
typedef signed __int64 i64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#ifdef _WIN64
typedef i64 imm;
typedef u64 umm;
#else
typedef i32 imm;
typedef u32 umm;
#endif

typedef float f32;
typedef double f64;

typedef u8 bool;
#define true 1
#define false 0

#define ERR_CODE (-(__COUNTER__))

bool GlobalRunning = false;
HBITMAP Bitmap  = 0;

LRESULT CALLBACK
WindowProc(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    
    if (message == WM_QUIT || message == WM_CLOSE) GlobalRunning = false;
    
    else if (message == WM_DROPFILES)
    {
        u32 required_chars = DragQueryFileA((HDROP)wparam, 0, 0, 0) + 1;
        
        void* file_name = VirtualAlloc(0, required_chars * sizeof(WCHAR), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        DragQueryFileW((HDROP)wparam, 0, file_name, required_chars);
        
        DeleteObject(Bitmap);
        Bitmap = LoadImageW(0, file_name, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        
        if (Bitmap == 0) MessageBeep(MB_ICONERROR);
        else InvalidateRect(window_handle, 0, 0);
    }
    
    else if (message == WM_SIZE || message == WM_SIZING)
    {
        InvalidateRect(window_handle, 0, 0);
        UpdateWindow(window_handle);
    }
    
    else if (message == WM_PAINT)
    {
        PAINTSTRUCT p = {0};
        HDC dc = BeginPaint(window_handle, &p);
        
        RECT client_rect;
        GetClientRect(window_handle, &client_rect);
        u32 client_width  = client_rect.right  - client_rect.left;
        u32 client_height = client_rect.bottom - client_rect.top;
        
        FillRect(dc, &client_rect, GetStockObject(BLACK_BRUSH));
        
        SetBkColor(dc, 0);
        SetTextColor(dc, 0x00FFFFFF);
        
        if (Bitmap == 0)
        {
            HFONT font = CreateFontW(26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,L"SYSTEM_FIXED_FONT");
            HFONT prev_font = SelectObject(dc, font);
            
            SetTextAlign(dc, TA_CENTER);
            TextOutW(dc, client_width / 2, client_height / 2,
                     L"I eat .bmp files for breakfast", sizeof("I eat .bmp files for breakfast") - 1);
            
            DeleteObject(SelectObject(dc, prev_font));
        }
        
        else
        {
            HFONT font = CreateFontW(5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,L"SYSTEM_FIXED_FONT");
            HFONT prev_font = SelectObject(dc, font);
            
            TEXTMETRICW metrics;
            GetTextMetrics(dc, &metrics);
            
            int char_size = metrics.tmAveCharWidth;
            if (char_size < metrics.tmHeight) char_size = metrics.tmHeight;
            
            BITMAP bitmap_info;
            GetObject(Bitmap, sizeof(BITMAP), &bitmap_info);
            
            SetWindowPos(window_handle, 0, 0, 0, char_size * bitmap_info.bmWidth, char_size * bitmap_info.bmHeight + GetSystemMetrics(SM_CYCAPTION), SWP_NOMOVE | SWP_NOREPOSITION);
            
            WCHAR palette[] = L" .:;~=#OB8%&";
            
            for (umm y = 0; y < bitmap_info.bmHeight; ++y)
            {
                for (umm x = 0; x < bitmap_info.bmWidth; ++x)
                {
                    u8* color_ptr = (u8*)bitmap_info.bmBits + (bitmap_info.bmHeight - (y + 1)) * bitmap_info.bmWidthBytes + x * 3;
                    
                    f32 luminosity = (0.2126f*color_ptr[0] + 0.7152f*color_ptr[1] + 0.0722f*color_ptr[2]) / 255;
                    
                    f32 brightness = 3*(luminosity * luminosity) - 2*(luminosity * luminosity * luminosity);
                    
                    umm slot = (umm)(brightness * ((sizeof(palette) / sizeof(palette[0])) - 1) + 0.5f);
                    if (slot > (sizeof(palette) / sizeof(palette[0])) - 1) *(volatile int*)0;
                    
                    TextOutW(dc, (int)(x * char_size + char_size / 2), (int)(y * char_size + char_size / 2),
                             &palette[slot], 1);
                }
            }
            
            DeleteObject(SelectObject(dc, prev_font));
        }
        
        EndPaint(window_handle, &p);
    }
    
    else result = DefWindowProc(window_handle, message, wparam, lparam);
    
    return result;
}

int WINAPI
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR arguments, int visibility)
{
    int return_code = 0;
    
    instance = GetModuleHandle(0);
    
    LPWSTR name = L"asciify";
    
    WNDCLASSW window_class = {
        .style         = 0,
        .lpfnWndProc   = &WindowProc,
        .cbClsExtra    = 0,
        .cbWndExtra    = 0,
        .hInstance     = instance,
        .lpszMenuName  = 0,
        .lpszClassName = name,
    };
    
    if (!RegisterClassW(&window_class)) return_code = ERR_CODE;
    else
    {
        HWND window_handle = CreateWindowExW(0, name, name, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                             0, 0, instance, 0);
        
        if (!window_handle) return_code = ERR_CODE;
        else
        {
            DragAcceptFiles(window_handle, true);
            
            GlobalRunning = true;
            while (GlobalRunning)
            {
                MSG message;
                while (PeekMessageW(&message, window_handle, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
            }
        }
    }
    
    return return_code;
}