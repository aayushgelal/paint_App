#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define TOOL_PENCIL 0
#define TOOL_RECT   1

#define COLOR_BLACK 0xFF000000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_RED   0xFFFF0000
#define COLOR_GREEN 0xFF00FF00
#define COLOR_BLUE  0xFF0000FF

typedef struct {
    SDL_Window* win;
    SDL_Surface* screen;
    SDL_Surface* img;
    SDL_Surface* overlay;
    uint32_t color;
    int tool;
    int mouse_down;
    int win_width;
    int win_height;
    int x;
    int y;
} mage_state;

/* probably going to want to add stuff before exit later */
void mage_exit(int code)
{
    exit(code);
}

void mage_error(const char* str)
{
    printf("ERROR: %s", str);
    mage_exit(1);
}

SDL_Surface* mage_load_image(char* f)
{
    SDL_Surface* tmp;
    SDL_Surface* surface;

    printf("Loading image %s.\n", f);
    
    tmp = IMG_Load(f);
    if (tmp == NULL) {
        mage_error("Failed to load image.");
    }

    surface = SDL_ConvertSurfaceFormat(tmp, SDL_PIXELFORMAT_BGRA32, NULL);
    if (surface == NULL) {
        mage_error("Failed to convert image to BGRA32.");
    }

    SDL_FreeSurface(tmp);
    return surface;
}

void mage_pencil(mage_state* m, int x, int y)
{
    /* warning: endianness is ABGR */
    uint32_t* pixels = m->img->pixels;
    if (SDL_MUSTLOCK(m->img)) {
        SDL_LockSurface(m->img);
    }
    /* adjust for status bar (hack!) */
    y -= 16;

    pixels[(y*m->img->w)+x] = m->color;
    if (SDL_MUSTLOCK(m->img)) {
        SDL_UnlockSurface(m->img);
    }
}

void mage_rect(mage_state* m, int x, int y, int w, int h)
{
    /* warning: endianness is ABGR */
    uint32_t* pixels = m->img->pixels;
    SDL_Rect rect;
    if (SDL_MUSTLOCK(m->img)) {
        SDL_LockSurface(m->img);
    }
    /* adjust for status bar (hack!) */
    y -= 16;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_FillRect(m->img, &rect, m->color);
    if (SDL_MUSTLOCK(m->img)) {
        SDL_UnlockSurface(m->img);
    }
}

void mage_render(mage_state* m)
{
    /* creen clear and status bar */
    SDL_Rect rect;
    SDL_FillRect(m->screen, NULL, COLOR_BLACK);
    rect.w = 16;
    rect.h = 16;
    rect.x = 0;
    rect.y = 0;
    SDL_FillRect(m->screen, &rect, m->color);
    rect.y = -16;

    rect.w = m->img->w;
    rect.h = m->img->h;
    SDL_BlitSurface(m->img, &rect, m->screen, NULL);
    rect.w = m->overlay->w;
    rect.h = m->overlay->h;
    SDL_BlitSurface(m->overlay, &rect, m->screen, NULL);
    SDL_UpdateWindowSurface(m->win);
}

mage_state* mage_init(void)
{
    mage_state* m = malloc(sizeof(*m));
    if (m == NULL) {
        mage_error("Failed to initialize state!");
    }

    m->color = COLOR_BLACK;
    m->tool = TOOL_PENCIL;
    m->mouse_down = 0;
    m->win_width = 640;
    m->win_height = 480;
    m->x = 0;
    m->y = 0;

    return m;
}

int main(int argc, char* argv[])
{
    mage_state* m = mage_init();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        mage_error("Failed to initialize SDL!");
    }

    m->win = SDL_CreateWindow("mage", 0, 0, m->win_width, m->win_height, SDL_WINDOW_SHOWN);
    if (m->win == NULL) {
        mage_error("Failed to initialize SDL window!");
    }

    m->screen = SDL_GetWindowSurface(m->win);

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        mage_error("Failed to initialize SDL_image!");
    }

    if (argc > 1) {
        m->img = mage_load_image(argv[1]);
        if (m->img == NULL) {
            mage_error("Failed to initialize image surface!");
        }
    } else {
        /* fall back to a blank canvas */
        m->img = SDL_CreateRGBSurfaceWithFormat(NULL,
                                             m->win_width,
                                             m->win_height,
                                             8,
                                             SDL_PIXELFORMAT_BGRA32);
        SDL_FillRect(m->img, NULL, COLOR_WHITE);
        if (m->img == NULL) {
            mage_error("Failed to initialize image surface!");
        }
    }

    m->overlay = SDL_CreateRGBSurfaceWithFormat(NULL,
                                             m->img->w,
                                             m->img->h,
                                             8,
                                             SDL_PIXELFORMAT_BGRA32);
    if (m->overlay == NULL) {
        mage_error("Failed to initialize m->overlay surface!");
    }


    /* so we don't start with a black m->screen */
    mage_render(m);
    while (1) {
        SDL_Event e;
        int should_draw = 0;

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                mage_exit(0);
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_q || e.key.keysym.sym == SDLK_ESCAPE) {
                    mage_exit(0);
                }
                if (e.key.keysym.sym == SDLK_b) {
                    m->tool = TOOL_PENCIL;
                    should_draw = 1;
                }
                if (e.key.keysym.sym == SDLK_r) {
                    m->tool = TOOL_RECT;
                    should_draw = 1;
                }
                if (e.key.keysym.sym == SDLK_1) {
                    m->color = COLOR_BLACK;
                    should_draw = 1;
                }
                if (e.key.keysym.sym == SDLK_2) {
                    m->color = COLOR_WHITE;
                    should_draw = 1;
                }
                if (e.key.keysym.sym == SDLK_3) {
                    m->color = COLOR_RED;
                    should_draw = 1;
                }
                if (e.key.keysym.sym == SDLK_4) {
                    m->color = COLOR_GREEN;
                    should_draw = 1;
                }
                if (e.key.keysym.sym == SDLK_5) {
                    m->color = COLOR_BLUE;
                    should_draw = 1;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    switch (m->tool) {
                    case TOOL_PENCIL:
                        mage_pencil(m, e.button.x, e.button.y);
                        break;
                    case TOOL_RECT:
                        m->x = e.button.x;
                        m->y = e.button.y;
                        break;
                    }
                    m->mouse_down = 1;
                    should_draw = 1;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (m->mouse_down && e.button.button == SDL_BUTTON_LEFT) {
                    switch (m->tool) {
                    case TOOL_RECT:
                        mage_rect(m,
                                  m->x<e.button.x?m->x:e.button.x,
                                  m->y<e.button.y?m->y:e.button.y,
                                  m->x<e.button.x?e.button.x-m->x:m->x-e.button.x,
                                  m->y<e.button.y?e.button.y-m->y:m->y-e.button.y);
                        break;
                    }
                    should_draw = 1;
                }
                m->mouse_down = 0;
                should_draw = 1;
                break;
            case SDL_MOUSEMOTION:
                if (m->mouse_down && e.button.button == SDL_BUTTON_LEFT) {
                    switch (m->tool) {
                    case TOOL_PENCIL:
                        mage_pencil(m, e.button.x, e.button.y);
                        break;
                    case TOOL_RECT: {
                        /* adjust for status bar (hack!) */
                        int motion_x = e.motion.x;
                        int motion_y = e.motion.y-16;
                        int xx = m->x;
                        int yy = m->y-16;
                        SDL_Rect rect;
                        rect.x = xx<motion_x?xx:motion_x;
                        rect.y = yy<motion_y?yy:motion_y;
                        rect.w = xx<motion_x?motion_x-xx:xx-motion_x;
                        rect.h = yy<motion_y?motion_y-yy:yy-motion_y;
                        SDL_FillRect(m->overlay, &rect, m->color);
                        break;
                    }
                    }
                    should_draw = 1;
                }
                break;
            default:
                break;
            }
        }
        if (should_draw) {
            mage_render(m);
            SDL_FillRect(m->overlay, NULL, 0x00000000);
        }
    }
    return 0;
}
