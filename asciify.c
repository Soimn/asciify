#define UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
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
HBITMAP Bitmap     = 0;
HBITMAP Backbuffer = 0;
u32 OriginalWidth  = 0;
u32 OriginalHeight = 0;
bool ColorOn       = false;

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
        else
        {
            DeleteObject(Backbuffer);
            Backbuffer = 0;
            
            InvalidateRect(window_handle, 0, 0);
        }
    }
    
    else if (message == WM_KEYDOWN && wparam == 'N' && (GetKeyState(VK_CONTROL) & 0x8000) != 0)
    {
        DeleteObject(Bitmap);
        DeleteObject(Backbuffer);
        Bitmap     = 0;
        Backbuffer = 0;
        SetWindowPos(window_handle, 0, 0, 0, OriginalWidth, OriginalHeight, SWP_NOMOVE | SWP_NOREPOSITION);
        InvalidateRect(window_handle, 0, 0);
    }
    
    else if (message == WM_KEYDOWN && wparam == 'X' && (GetKeyState(VK_CONTROL) & 0x8000) != 0)
    {
        ColorOn = !ColorOn;
        
        DeleteObject(Backbuffer);
        Backbuffer = 0;
        InvalidateRect(window_handle, 0, 0);
    }
    
    else if (message == WM_KEYDOWN && wparam == 'S' && (GetKeyState(VK_CONTROL) & 0x8000) != 0 && Bitmap != 0)
    {
        // This is incredibly jank, but it works
        WCHAR path[1024] = {0};
        LPWSTR path_wstr = path;
        
        OPENFILENAMEW open_file_name = {
            .lStructSize    = sizeof(OPENFILENAMEW),
            .hwndOwner      = window_handle,
            .hInstance      = GetModuleHandleW(0),
            .lpstrFilter    = L"Bitmap\0*.bmp\0\0",
            .nFilterIndex   = 1,
            .lpstrFile      = path_wstr,
            .nMaxFile       = sizeof(path) / sizeof(WCHAR),
            .lpstrTitle     = L"Save resulting bitmap",
            .lpstrDefExt    = L".bmp",
        };
        
        GetSaveFileNameW(&open_file_name);
        
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        
        if (file != INVALID_HANDLE_VALUE)
        {
            HDC dc = GetDC(window_handle);
            
            HDC bb_dc = CreateCompatibleDC(dc);
            
            RECT client_rect;
            GetClientRect(window_handle, &client_rect);
            
            HBITMAP window = CreateCompatibleBitmap(dc, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);
            HBITMAP prev_bitmap = SelectObject(bb_dc, window);
            BitBlt(bb_dc, 0, 0, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, dc, 0, 0, SRCCOPY);
            
            BITMAP window_bitmap;
            GetObject(window, sizeof(BITMAP), &window_bitmap);
            
            BITMAPINFOHEADER bitmap_info = {
                .biSize        = sizeof(BITMAPINFOHEADER),
                .biWidth       = window_bitmap.bmWidth,
                .biHeight      = window_bitmap.bmHeight,
                .biPlanes      = 1,
                .biBitCount    = 24,
                .biCompression = BI_RGB,
            };
            
            u32 bitmap_size = ((window_bitmap.bmWidth * bitmap_info.biBitCount + 31) / 32) * 4 * window_bitmap.bmHeight;
            
            void* bitmap_memory = VirtualAlloc(0, bitmap_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            GetDIBits(dc, window, 0, window_bitmap.bmHeight, bitmap_memory, (BITMAPINFO*)&bitmap_info, DIB_RGB_COLORS);
            
            BITMAPFILEHEADER file_header = {
                .bfType    = 0x4D42,
                .bfSize    = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bitmap_size,
                .bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER)
            };
            
            DWORD _bw;
            WriteFile(file, (LPSTR)&file_header, sizeof(BITMAPFILEHEADER), &_bw, 0);
            WriteFile(file, (LPSTR)&bitmap_info, sizeof(BITMAPINFOHEADER), &_bw, 0);
            WriteFile(file, (LPSTR)bitmap_memory, bitmap_size, &_bw, 0);
            
            DeleteObject(window);
            DeleteObject(bb_dc);
            ReleaseDC(window_handle, dc);
            
            SelectObject(bb_dc, prev_bitmap);
            
            CloseHandle(file);
        }
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
            if (Backbuffer != 0)
            {
                HDC bb_dc = CreateCompatibleDC(dc);
                HBITMAP prev_bitmap = SelectObject(bb_dc, Backbuffer);
                
                BITMAP backbuffer_info;
                GetObject(Backbuffer, sizeof(BITMAP), &backbuffer_info);
                
                BitBlt(dc, 0, 0, backbuffer_info.bmWidth, backbuffer_info.bmHeight, bb_dc, 0, 0, SRCCOPY);
                
                SelectObject(bb_dc, prev_bitmap);
                DeleteDC(bb_dc);
            }
            
            else
            {
                BITMAP bitmap_info;
                GetObject(Bitmap, sizeof(BITMAP), &bitmap_info);
                
                HFONT font = CreateFontW(5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,L"SYSTEM_FIXED_FONT");
                HFONT prev_font = SelectObject(dc, font);
                
                TEXTMETRICW metrics;
                GetTextMetrics(dc, &metrics);
                
                int char_size = metrics.tmAveCharWidth;
                if (char_size < metrics.tmHeight) char_size = metrics.tmHeight;
                
                SetWindowPos(window_handle, 0, 0, 0, char_size * bitmap_info.bmWidth, char_size * bitmap_info.bmHeight + GetSystemMetrics(SM_CYCAPTION), SWP_NOMOVE | SWP_NOREPOSITION);
                
                GetClientRect(window_handle, &client_rect);
                client_width  = client_rect.right  - client_rect.left;
                client_height = client_rect.bottom - client_rect.top;
                
                DeleteObject(SelectObject(dc, prev_font));
                
                HDC bb_dc = CreateCompatibleDC(dc);
                
                font = CreateFontW(5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,L"SYSTEM_FIXED_FONT");
                prev_font = SelectObject(bb_dc, font);
                
                Backbuffer = CreateCompatibleBitmap(dc, client_width, client_height);
                HBITMAP prev_bitmap = SelectObject(bb_dc, Backbuffer);
                
                SetBkColor(bb_dc, 0);
                SetTextColor(bb_dc, 0x00FFFFFF);
                
                WCHAR palette[] = L" .:;~=#OB8%&";
                
                for (umm y = 0; y < bitmap_info.bmHeight; ++y)
                {
                    for (umm x = 0; x < bitmap_info.bmWidth; ++x)
                    {
                        u8* color_ptr = (u8*)bitmap_info.bmBits + (bitmap_info.bmHeight - (y + 1)) * bitmap_info.bmWidthBytes + x * 3;
                        if (ColorOn)
                        {
                            f32 r = color_ptr[2] / 255.0f;
                            f32 g = color_ptr[1] / 255.0f;
                            f32 b = color_ptr[0] / 255.0f;
                            
                            r = 3*(r*r) - 2*(r*r*r);
                            g = 3*(g*g) - 2*(g*g*g);
                            b = 3*(b*b) - 2*(b*b*b);
                            
                            SetTextColor(bb_dc, (u32)(b*255) << 16 | (u32)(g*255) << 8 | (u32)(r*255));
                        }
                        
                        f32 luminosity = (0.2126f*color_ptr[2] + 0.7152f*color_ptr[1] + 0.0722f*color_ptr[0]) / 255;
                        
                        f32 brightness = 3*(luminosity * luminosity) - 2*(luminosity * luminosity * luminosity);
                        
                        umm slot = (umm)(brightness * ((sizeof(palette) / sizeof(palette[0]) - 1) - 1) + 0.5f);
                        if (slot > (sizeof(palette) / sizeof(palette[0]) - 1) - 1) *(volatile int*)0;
                        
                        TextOutW(bb_dc, (int)(x * char_size + char_size / 2), (int)(y * char_size + char_size / 2),
                                 &palette[slot], 1);
                    }
                }
                
                DeleteObject(SelectObject(bb_dc, prev_font));
                
                SelectObject(bb_dc, prev_bitmap);
                DeleteDC(bb_dc);
                
                InvalidateRect(window_handle, 0, false);
                UpdateWindow(window_handle);
            }
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
            
            RECT window_rect;
            GetWindowRect(window_handle, &window_rect);
            OriginalWidth  = window_rect.right  - window_rect.left;
            OriginalHeight = window_rect.bottom - window_rect.top;
            
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