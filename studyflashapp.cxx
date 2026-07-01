/*
 * StudyFlash -- redesigned SDL2 flashcard app
 * Android/CxxDroid fullscreen build
 *
 * Build:
 *   g++ studyflash.cpp -o studyflash \
 *       $(sdl2-config --cflags --libs) -lSDL2_ttf -lm -std=c++17
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Palette (light theme)
static const SDL_Color C_BG = {248, 246, 242, 255};
static const SDL_Color C_TEXT = {26, 26, 46, 255};
static const SDL_Color C_MUTED = {107, 114, 128, 255};
static const SDL_Color C_INDIGO = {91, 95, 239, 255};
static const SDL_Color C_CORAL = {255, 107, 107, 255};
static const SDL_Color C_CARD = {255, 255, 255, 255};
static const SDL_Color C_BORDER = {229, 228, 240, 255};

struct TopicStyle
{
    SDL_Color chip;
    SDL_Color text;
};
static const TopicStyle TOPIC_STYLES[] = {
    {{225, 226, 252, 255}, {91, 95, 239, 255}},
    {{255, 235, 235, 255}, {220, 80, 80, 255}},
    {{212, 246, 236, 255}, {22, 163, 111, 255}},
    {{255, 243, 213, 255}, {202, 138, 4, 255}},
    {{237, 233, 254, 255}, {109, 40, 217, 255}},
    {{219, 244, 255, 255}, {14, 116, 180, 255}},
};

struct FlashCard
{
    std::string question;
    std::string answer;
};
struct Topic
{
    std::string name;
    std::string tag;
    std::vector<FlashCard> cards;
};

static std::vector<Topic> TOPICS = {
    {"C++ Basics", "[CPP]", {
                                {"What is C++?", "A general-purpose, compiled language with object-oriented, generic, and low-level memory features."},
                                {"What is a variable?", "A named storage location in memory that holds a value of a specific type."},
                                {"What is a function?", "A named, reusable block of code that performs a specific task and can optionally return a value."},
                                {"What is a pointer?", "A variable that stores the memory address of another variable, enabling direct memory access."},
                                {"What is a reference?", "An alias for an existing variable -- same memory, different name. Cannot be reseated after init."},
                            }},
    {"OOP", "[OOP]", {
                         {"What is OOP?", "Object-Oriented Programming -- organises code into objects combining data and behaviour."},
                         {"What is encapsulation?", "Bundling data and methods inside a class, hiding internal state from outside."},
                         {"What is inheritance?", "A derived class acquires properties and behaviours from a base class."},
                         {"What is polymorphism?", "Different types treated through a common interface -- one interface, many implementations."},
                         {"What is abstraction?", "Hiding complex implementation details, exposing only the necessary interface."},
                     }},
    {"Memory", "[MEM]", {
                            {"What is the heap?", "Dynamically allocated memory managed manually (new/delete) or via smart pointers."},
                            {"What is the stack?", "Automatically managed memory for local variables. Freed when function returns (LIFO)."},
                            {"What is RAII?", "Resource Acquisition Is Initialisation -- tie resource lifetime to object lifetime."},
                            {"What is a smart pointer?", "Wrapper managing heap memory: unique_ptr (sole), shared_ptr (shared), weak_ptr."},
                        }},
    {"STL", "[STL]", {
                         {"What is the STL?", "Standard Template Library -- generic containers, algorithms, and iterators."},
                         {"vector vs array?", "vector is dynamic heap-allocated; array is fixed-size stack-allocated."},
                         {"What is an iterator?", "An object that traverses a container generically, like a type-safe pointer."},
                         {"What is std::move?", "Transfers ownership of resources without copying (enables move semantics)."},
                     }},
    {"Algorithms", "[ALG]", {
                                {"What is Big-O?", "Describes algorithm time/space complexity as input size grows: O(n), O(log n), etc."},
                                {"What is binary search?", "Finds a target in a sorted array in O(log n) by halving the range each step."},
                                {"What is quicksort?", "Divide-and-conquer sort: pick pivot, partition, recurse. Avg O(n log n)."},
                                {"What is memoisation?", "Caching expensive function results so repeated calls return instantly."},
                            }},
    {"Modern C++", "[MOD]", {
                                {"What is auto?", "Compiler deduces the variable type from the initialiser -- reduces verbosity."},
                                {"What are lambdas?", "Anonymous inline functions: [capture](params){ body }. First-class in C++11+."},
                                {"What is constexpr?", "Directs the compiler to evaluate an expression at compile-time."},
                                {"What are concepts?", "C++20 named constraints on template parameters -- cleaner errors and interfaces."},
                            }},
};

enum Page
{
    PAGE_HOME,
    PAGE_CARD
};

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static bool ptInRect(int x, int y, SDL_Rect r)
{
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

static void fillRoundRect(SDL_Renderer *rnd, SDL_Rect rc, int radius,
                          Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (rc.w <= 0 || rc.h <= 0)
        return;
    radius = std::min(radius, std::min(rc.w / 2, rc.h / 2));
    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rnd, r, g, b, a);
    SDL_Rect mid = {rc.x, rc.y + radius, rc.w, rc.h - 2 * radius};
    SDL_Rect top = {rc.x + radius, rc.y, rc.w - 2 * radius, radius};
    SDL_Rect bot = {rc.x + radius, rc.y + rc.h - radius, rc.w - 2 * radius, radius};
    SDL_RenderFillRect(rnd, &mid);
    SDL_RenderFillRect(rnd, &top);
    SDL_RenderFillRect(rnd, &bot);
    for (int dy = 0; dy < radius; dy++)
    {
        for (int dx = 0; dx < radius; dx++)
        {
            float d = sqrtf((float)((dx - radius + 1) * (dx - radius + 1) + (dy - radius + 1) * (dy - radius + 1)));
            if (d <= (float)radius)
            {
                SDL_RenderDrawPoint(rnd, rc.x + dx, rc.y + dy);
                SDL_RenderDrawPoint(rnd, rc.x + rc.w - 1 - dx, rc.y + dy);
                SDL_RenderDrawPoint(rnd, rc.x + dx, rc.y + rc.h - 1 - dy);
                SDL_RenderDrawPoint(rnd, rc.x + rc.w - 1 - dx, rc.y + rc.h - 1 - dy);
            }
        }
    }
}

static void drawShadow(SDL_Renderer *rnd, SDL_Rect rc, int radius)
{
    for (int i = 3; i >= 0; i--)
    {
        SDL_Rect s = {rc.x - i + 3, rc.y - i + 4, rc.w + 2 * i, rc.h + 2 * i};
        fillRoundRect(rnd, s, radius + i, 0, 0, 0, (Uint8)(12 - i * 2));
    }
}

struct TextSize
{
    int w, h;
};

static TextSize renderText(SDL_Renderer *rnd, TTF_Font *font,
                           const std::string &text, SDL_Color col,
                           int x, int y, int wrapW, bool centered = false,
                           Uint8 alpha = 255)
{
    if (text.empty())
        return {0, 0};
    SDL_Surface *s = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), col, wrapW);
    if (!s)
        return {0, 0};
    SDL_Texture *tex = SDL_CreateTextureFromSurface(rnd, s);
    SDL_FreeSurface(s);
    if (!tex)
        return {0, 0};
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_SetTextureAlphaMod(tex, alpha);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_Rect dst = {centered ? x - w / 2 : x, y, w, h};
    SDL_RenderCopy(rnd, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    return {w, h};
}

struct ScrollState
{
    float offset = 0, velocity = 0, maxOffset = 0, dragStartOffset = 0;
    int dragStartY = 0;
    bool dragging = false;
};

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << SDL_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() < 0)
    {
        std::cerr << TTF_GetError() << "\n";
        return 1;
    }

    // --- FIX 1: true fullscreen, read real dimensions back ---
    SDL_Window *win = SDL_CreateWindow("StudyFlash",
                                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       0, 0,
                                       SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win)
    {
        std::cerr << SDL_GetError() << "\n";
        return 1;
    }

    int WIN_W = 0, WIN_H = 0;
    SDL_GetWindowSize(win, &WIN_W, &WIN_H);

    SDL_Renderer *rnd = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!rnd)
    {
        std::cerr << SDL_GetError() << "\n";
        return 1;
    }
    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);

    // --- FIX 2: pre-fill both back buffers to kill the initial flicker ---
    SDL_SetRenderDrawColor(rnd, C_BG.r, C_BG.g, C_BG.b, 255);
    SDL_RenderClear(rnd);
    SDL_RenderPresent(rnd);
    SDL_RenderClear(rnd);
    SDL_RenderPresent(rnd);

    // --- FIX 3: font sizes scale with screen density ---
    float density = WIN_W / 400.0f;
    auto openFont = [](const char *name, int size) -> TTF_Font *
    {
        const char *dirs[] = {
            "/usr/share/fonts/truetype/dejavu/",
            "/usr/share/fonts/truetype/liberation/",
            "",
            nullptr};

        for (int i = 0; dirs[i]; i++)
        {
            std::string path = std::string(dirs[i]) + name;

            std::cout << "Trying: " << path << std::endl;

            TTF_Font *f = TTF_OpenFont(path.c_str(), size);

            if (f)
            {
                std::cout << "SUCCESS: " << path << std::endl;
                return f;
            }
        }

        return nullptr;
    };

    TTF_Font *fDisplay = openFont("DejaVuSans-Bold.ttf", (int)(34 * density));
    TTF_Font *fH1 = openFont("DejaVuSans-Bold.ttf", (int)(26 * density));
    TTF_Font *fH2 = openFont("DejaVuSans-Bold.ttf", (int)(20 * density));
    TTF_Font *fBody = openFont("DejaVuSans.ttf", (int)(18 * density));
    TTF_Font *fCaption = openFont("DejaVuSans.ttf", (int)(14 * density));
    TTF_Font *fLabel = openFont("DejaVuSans-Bold.ttf", (int)(13 * density));

    if (!fDisplay || !fH1 || !fH2 || !fBody || !fCaption || !fLabel)
    {
        std::cerr << "Font load failed: " << TTF_GetError() << "\n";
        return 1;
    }

    // State
    Page page = PAGE_HOME;
    int activeTopic = -1;
    int curCard = 0;
    bool showAnswer = false;
    bool running = true;

    float ansAlpha = 0;
    float cardOffX = 0, cardTargetX = 0;
    bool inTrans = false;
    int nextCard = 0;
    float pageSlide = 0, pageTarget = 0;
    int hoverTopic = -1;
    ScrollState scroll;

    // --- FIX 4: all layout constants computed AFTER we know WIN_W/WIN_H ---
    const int M = (int)(WIN_W * 0.05f);
    const int COLS = 2;
    const int CHIP_GAP = (int)(WIN_W * 0.03f);
    const int CHIP_W = (WIN_W - 2 * M - (COLS - 1) * CHIP_GAP) / COLS;
    const int CHIP_H = (int)(WIN_H * 0.13f);
    const int CHIP_R = (int)(CHIP_H * 0.16f);
    const int HDR_H = (int)(WIN_H * 0.22f);
    const int GRID_TOP = HDR_H + M;
    const int NTOPICS = (int)TOPICS.size();
    const int NROWS = (NTOPICS + COLS - 1) / COLS;
    const int GRID_H = NROWS * (CHIP_H + CHIP_GAP) - CHIP_GAP;
    const int TOTAL_H = GRID_TOP + GRID_H + 60;
    const int CARD_R = (int)(WIN_W * 0.06f);
    const int BTN_H = (int)(WIN_H * 0.08f);
    const int BTN_Y = WIN_H - BTN_H - M;
    const int TOP_H = (int)(WIN_H * 0.09f); // card page top strip height

    Uint32 lastMs = SDL_GetTicks();

    while (running)
    {
        Uint32 now = SDL_GetTicks();
        float dt = std::min((float)(now - lastMs) / 1000.f, 0.05f);
        lastMs = now;

        // Animate
        pageSlide = lerpf(pageSlide, pageTarget, 1.f - powf(0.01f, dt * 6));
        if (fabsf(pageSlide - pageTarget) < 0.5f)
            pageSlide = pageTarget;

        if (inTrans)
        {
            cardOffX = lerpf(cardOffX, cardTargetX, 1.f - powf(0.001f, dt * 7));
            if (fabsf(cardOffX - cardTargetX) < 2.f)
            {
                cardOffX = 0;
                curCard = nextCard;
                inTrans = false;
            }
        }

        float alphaTarget = showAnswer ? 255.f : 0.f;
        ansAlpha = std::max(0.f, std::min(255.f,
                                          lerpf(ansAlpha, alphaTarget, 1.f - powf(0.001f, dt * 8))));

        if (!scroll.dragging)
        {
            scroll.offset += scroll.velocity * dt;
            scroll.velocity *= powf(0.01f, dt * 3);
            scroll.maxOffset = std::max(0.f, (float)(TOTAL_H - WIN_H));
            scroll.offset = std::max(0.f, std::min(scroll.offset, scroll.maxOffset));
            if (scroll.offset <= 0 || scroll.offset >= scroll.maxOffset)
                scroll.velocity = 0;
        }

        // Events
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                running = false;
                break;
            }

            // Android back button
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_AC_BACK)
            {
                if (page == PAGE_CARD)
                {
                    page = PAGE_HOME;
                    pageTarget = 0;
                }
                else
                    running = false;
            }

            if (page == PAGE_HOME)
            {
                if (ev.type == SDL_MOUSEMOTION)
                {
                    hoverTopic = -1;
                    for (int i = 0; i < NTOPICS; i++)
                    {
                        int cx = M + (i % COLS) * (CHIP_W + CHIP_GAP);
                        int cy = GRID_TOP - (int)scroll.offset + (i / COLS) * (CHIP_H + CHIP_GAP);
                        if (ptInRect(ev.motion.x, ev.motion.y, {cx, cy, CHIP_W, CHIP_H}))
                        {
                            hoverTopic = i;
                            break;
                        }
                    }
                }
                if (ev.type == SDL_MOUSEBUTTONDOWN)
                {
                    scroll.dragging = true;
                    scroll.dragStartY = ev.button.y;
                    scroll.dragStartOffset = scroll.offset;
                    scroll.velocity = 0;
                }
                if (ev.type == SDL_MOUSEMOTION && scroll.dragging)
                {
                    scroll.offset = std::max(0.f, std::min(scroll.maxOffset,
                                                           scroll.dragStartOffset + (float)(scroll.dragStartY - ev.motion.y)));
                }
                if (ev.type == SDL_MOUSEBUTTONUP)
                {
                    scroll.dragging = false;
                    if (abs(ev.button.y - scroll.dragStartY) < 12)
                    {
                        for (int i = 0; i < NTOPICS; i++)
                        {
                            int cx = M + (i % COLS) * (CHIP_W + CHIP_GAP);
                            int cy = GRID_TOP - (int)scroll.offset + (i / COLS) * (CHIP_H + CHIP_GAP);
                            if (ptInRect(ev.button.x, ev.button.y, {cx, cy, CHIP_W, CHIP_H}))
                            {
                                activeTopic = i;
                                curCard = 0;
                                showAnswer = false;
                                ansAlpha = 0;
                                cardOffX = 0;
                                inTrans = false;
                                page = PAGE_CARD;
                                pageTarget = (float)WIN_W;
                                break;
                            }
                        }
                    }
                }
                if (ev.type == SDL_MOUSEWHEEL)
                    scroll.velocity -= ev.wheel.y * 200.f;
            }
            else if (page == PAGE_CARD && fabsf(pageSlide - pageTarget) < 4.f)
            {
                int total = (int)TOPICS[activeTopic].cards.size();
                int bW = (WIN_W - 2 * M - 2 * CHIP_GAP) / 3;
                SDL_Rect bPrev = {M, BTN_Y, bW, BTN_H};
                SDL_Rect bShow = {M + bW + CHIP_GAP, BTN_Y, bW, BTN_H};
                SDL_Rect bNext = {M + 2 * (bW + CHIP_GAP), BTN_Y, bW, BTN_H};

                auto goNext = [&]()
                {
                    if (inTrans)
                        return;
                    nextCard = (curCard + 1) % total;
                    inTrans = true;
                    cardOffX = 0;
                    cardTargetX = -(float)WIN_W;
                    showAnswer = false;
                    ansAlpha = 0;
                };
                auto goPrev = [&]()
                {
                    if (inTrans)
                        return;
                    nextCard = (curCard - 1 + total) % total;
                    inTrans = true;
                    cardOffX = 0;
                    cardTargetX = (float)WIN_W;
                    showAnswer = false;
                    ansAlpha = 0;
                };

                if (ev.type == SDL_KEYDOWN)
                {
                    if (ev.key.keysym.sym == SDLK_SPACE)
                        showAnswer = !showAnswer;
                    if (ev.key.keysym.sym == SDLK_RIGHT)
                        goNext();
                    if (ev.key.keysym.sym == SDLK_LEFT)
                        goPrev();
                    if (ev.key.keysym.sym == SDLK_ESCAPE)
                    {
                        page = PAGE_HOME;
                        pageTarget = 0;
                    }
                }
                if (ev.type == SDL_MOUSEBUTTONDOWN)
                {
                    int mx = ev.button.x, my = ev.button.y;
                    // Back tap zone -- full top strip
                    if (ptInRect(mx, my, {0, 0, WIN_W / 3, TOP_H}))
                    {
                        page = PAGE_HOME;
                        pageTarget = 0;
                    }
                    else if (ptInRect(mx, my, bPrev))
                        goPrev();
                    else if (ptInRect(mx, my, bShow))
                        showAnswer = !showAnswer;
                    else if (ptInRect(mx, my, bNext))
                        goNext();
                    else
                    {
                        int cardY = TOP_H + 8;
                        int cardH = BTN_Y - cardY - M;
                        if (ptInRect(mx, my, {M, cardY, WIN_W - 2 * M, cardH}) && !inTrans)
                            showAnswer = !showAnswer;
                    }
                }
            }
        }

        // --- FIX 5: clear full screen to bg color every frame ---
        SDL_SetRenderDrawColor(rnd, C_BG.r, C_BG.g, C_BG.b, 255);
        SDL_RenderClear(rnd);

        int homeX = -(int)pageSlide;
        int cardPageX = WIN_W - (int)pageSlide;

        // ---- HOME PAGE ----
        if (homeX > -WIN_W && homeX < WIN_W)
        {
            SDL_Rect clip = {homeX, 0, WIN_W, WIN_H};
            SDL_RenderSetClipRect(rnd, &clip);

            // Header
            fillRoundRect(rnd, {homeX, 0, WIN_W, HDR_H}, 0, 91, 95, 239, 255);
            // Arch cutout at bottom of header
            fillRoundRect(rnd, {homeX - 40, HDR_H - 28, WIN_W + 80, 56}, 28,
                          C_BG.r, C_BG.g, C_BG.b, 255);

            renderText(rnd, fDisplay, "StudyFlash", {255, 255, 255, 255},
                       homeX + WIN_W / 2, (int)(HDR_H * 0.18f), WIN_W - 2 * M, true);
            renderText(rnd, fBody, "Pick a topic and start practising",
                       {200, 210, 252, 255}, homeX + WIN_W / 2, (int)(HDR_H * 0.50f), WIN_W - 2 * M, true);

            int totalCards = 0;
            for (auto &t : TOPICS)
                totalCards += (int)t.cards.size();
            std::string stat = std::to_string(totalCards) + " cards  |  " + std::to_string(TOPICS.size()) + " topics";
            renderText(rnd, fCaption, stat, {180, 200, 252, 255},
                       homeX + WIN_W / 2, (int)(HDR_H * 0.72f), WIN_W - 2 * M, true);

            // Topic grid
            for (int i = 0; i < NTOPICS; i++)
            {
                int cx = homeX + M + (i % COLS) * (CHIP_W + CHIP_GAP);
                int cy = GRID_TOP - (int)scroll.offset + (i / COLS) * (CHIP_H + CHIP_GAP);
                if (cy + CHIP_H < 0 || cy > WIN_H)
                    continue;

                const TopicStyle &ts = TOPIC_STYLES[i % 6];
                SDL_Rect chip = {cx, cy, CHIP_W, CHIP_H};
                drawShadow(rnd, chip, CHIP_R);
                Uint8 cr = ts.chip.r, cg = ts.chip.g, cb = ts.chip.b;
                if (hoverTopic == i)
                {
                    cr = (Uint8)std::max(0, cr - 20);
                    cg = (Uint8)std::max(0, cg - 20);
                }
                fillRoundRect(rnd, chip, CHIP_R, cr, cg, cb, 255);

                int tagY = cy + (int)(CHIP_H * 0.14f);
                int nameY = cy + (int)(CHIP_H * 0.44f);
                int cntY = cy + (int)(CHIP_H * 0.74f);
                renderText(rnd, fLabel, TOPICS[i].tag, ts.text, cx + M, tagY, CHIP_W - M);
                renderText(rnd, fH2, TOPICS[i].name, ts.text, cx + M, nameY, CHIP_W - M);
                std::string cnt = std::to_string(TOPICS[i].cards.size()) + " cards";
                renderText(rnd, fCaption, cnt,
                           {ts.text.r, ts.text.g, ts.text.b, 160}, cx + M, cntY, CHIP_W - M);
            }

            // Scroll bar
            if (scroll.maxOffset > 0)
            {
                float ratio = scroll.offset / scroll.maxOffset;
                int trkH = WIN_H - GRID_TOP;
                int barH = std::max(40, (int)(trkH * WIN_H / (float)TOTAL_H));
                int barY = GRID_TOP + (int)((trkH - barH) * ratio);
                SDL_SetRenderDrawColor(rnd, 91, 95, 239, 80);
                SDL_Rect sb = {homeX + WIN_W - 5, barY, 4, barH};
                SDL_RenderFillRect(rnd, &sb);
            }

            SDL_RenderSetClipRect(rnd, nullptr);
        }

        // ---- CARD PAGE ----
        if (cardPageX > -WIN_W && cardPageX < WIN_W && activeTopic >= 0)
        {
            SDL_Rect clip = {cardPageX, 0, WIN_W, WIN_H};
            SDL_RenderSetClipRect(rnd, &clip);

            const TopicStyle &ts = TOPIC_STYLES[activeTopic % 6];
            auto &topic = TOPICS[activeTopic];
            int total = (int)topic.cards.size();
            auto &card = topic.cards[curCard];
            int bW = (WIN_W - 2 * M - 2 * CHIP_GAP) / 3;

            SDL_Rect bPrev = {cardPageX + M, BTN_Y, bW, BTN_H};
            SDL_Rect bShow = {cardPageX + M + bW + CHIP_GAP, BTN_Y, bW, BTN_H};
            SDL_Rect bNext = {cardPageX + M + 2 * (bW + CHIP_GAP), BTN_Y, bW, BTN_H};

            // Top strip
            fillRoundRect(rnd, {cardPageX, 0, WIN_W, TOP_H}, 0,
                          ts.chip.r, ts.chip.g, ts.chip.b, 255);

            // Back label
            renderText(rnd, fH2, "< Topics", ts.text,
                       cardPageX + M, TOP_H / 2 - ((int)(20 * density) / 2), WIN_W / 3);
            // Topic name centred
            renderText(rnd, fH2, topic.name, ts.text,
                       cardPageX + WIN_W / 2, TOP_H / 2 - ((int)(20 * density) / 2), WIN_W - 2 * M, true);

            // Progress bar
            {
                int pbX = cardPageX + M, pbY = TOP_H - 6, pbW = WIN_W - 2 * M;
                SDL_SetRenderDrawColor(rnd, 255, 255, 255, 60);
                SDL_Rect pbBg = {pbX, pbY, pbW, 4};
                SDL_RenderFillRect(rnd, &pbBg);
                int fillW = (int)(pbW * (curCard + 1) / (float)total);
                SDL_SetRenderDrawColor(rnd, ts.text.r, ts.text.g, ts.text.b, 220);
                SDL_Rect pbFill = {pbX, pbY, fillW, 4};
                SDL_RenderFillRect(rnd, &pbFill);
            }

            // Card
            int cardY = TOP_H + M;
            int cardH = BTN_Y - cardY - M;
            int cardCX = cardPageX + M + (int)cardOffX;
            SDL_Rect cardRect = {cardCX, cardY, WIN_W - 2 * M, cardH};

            drawShadow(rnd, cardRect, CARD_R);
            fillRoundRect(rnd, cardRect, CARD_R, 255, 255, 255, 255);
            // Accent stripe at top of card
            fillRoundRect(rnd, {cardCX, cardY, cardRect.w, 6}, CARD_R,
                          ts.chip.r, ts.chip.g, ts.chip.b, 220);

            int innerX = cardCX + M + 8;
            int innerW = cardRect.w - 2 * (M + 8);

            // Counter badge
            {
                std::string ctr = std::to_string(curCard + 1) + " / " + std::to_string(total);
                int bx = cardCX + cardRect.w - (int)(WIN_W * 0.18f);
                int bw = (int)(WIN_W * 0.16f);
                fillRoundRect(rnd, {bx, cardY + M, bw, (int)(BTN_H * 0.45f)}, 12,
                              ts.chip.r, ts.chip.g, ts.chip.b, 180);
                renderText(rnd, fCaption, ctr, ts.text, bx + 8, cardY + M + 6, bw);
            }

            renderText(rnd, fLabel, "QUESTION", C_MUTED, innerX, cardY + M, innerW);
            auto qsz = renderText(rnd, fH1, card.question, C_TEXT,
                                  innerX, cardY + M + (int)(16 * density) + 8, innerW);

            int sepY = std::min(
                cardY + M + (int)(16 * density) + 8 + qsz.h + M,
                cardY + cardH / 2 + (int)(WIN_H * 0.03f));

            SDL_SetRenderDrawColor(rnd, C_BORDER.r, C_BORDER.g, C_BORDER.b, 255);
            SDL_RenderDrawLine(rnd, cardCX + M, sepY, cardCX + cardRect.w - M, sepY);

            if (ansAlpha < 10.f)
                renderText(rnd, fCaption, "Tap card or press Reveal",
                           C_MUTED, cardPageX + WIN_W / 2, sepY + 12, WIN_W - 2 * M, true);

            // Answer panel
            if (ansAlpha > 2.f)
            {
                Uint8 a = (Uint8)ansAlpha;
                SDL_SetRenderDrawColor(rnd, 255, 235, 235, a);
                SDL_Rect ap = {cardCX, sepY + 1,
                               cardRect.w, cardRect.y + cardRect.h - sepY - 1};
                SDL_RenderFillRect(rnd, &ap);
                SDL_SetRenderDrawColor(rnd, C_CORAL.r, C_CORAL.g, C_CORAL.b, a);
                SDL_Rect abar = {cardCX, sepY + 1, 5, ap.h};
                SDL_RenderFillRect(rnd, &abar);
                renderText(rnd, fLabel, "ANSWER",
                           {C_CORAL.r, C_CORAL.g, C_CORAL.b, a},
                           innerX + 8, sepY + M, innerW, false, a);
                renderText(rnd, fBody, card.answer,
                           {C_TEXT.r, C_TEXT.g, C_TEXT.b, a},
                           innerX + 8, sepY + M + (int)(14 * density) + 6, innerW - 8, false, a);
            }

            // Nav buttons
            drawShadow(rnd, bPrev, 12);
            fillRoundRect(rnd, bPrev, 14, C_BORDER.r, C_BORDER.g, C_BORDER.b, 255);
            renderText(rnd, fH2, "Prev", C_MUTED,
                       bPrev.x + bPrev.w / 2, bPrev.y + BTN_H / 4, bPrev.w, true);

            drawShadow(rnd, bShow, 12);
            if (showAnswer)
            {
                fillRoundRect(rnd, bShow, 14, C_CORAL.r, C_CORAL.g, C_CORAL.b, 255);
                renderText(rnd, fH2, "Hide", {255, 255, 255, 255},
                           bShow.x + bShow.w / 2, bShow.y + BTN_H / 4, bShow.w, true);
            }
            else
            {
                fillRoundRect(rnd, bShow, 14, 91, 95, 239, 255);
                renderText(rnd, fH2, "Reveal", {255, 255, 255, 255},
                           bShow.x + bShow.w / 2, bShow.y + BTN_H / 4, bShow.w, true);
            }

            drawShadow(rnd, bNext, 12);
            fillRoundRect(rnd, bNext, 14, C_BORDER.r, C_BORDER.g, C_BORDER.b, 255);
            renderText(rnd, fH2, "Next", C_MUTED,
                       bNext.x + bNext.w / 2, bNext.y + BTN_H / 4, bNext.w, true);

            SDL_RenderSetClipRect(rnd, nullptr);
        }

        SDL_RenderPresent(rnd);

        // --- FIX 6: cap frame rate so input stays responsive ---
        Uint32 elapsed = SDL_GetTicks() - now;
        if (elapsed < 16)
            SDL_Delay(16 - elapsed);
    }

    TTF_CloseFont(fDisplay);
    TTF_CloseFont(fH1);
    TTF_CloseFont(fH2);
    TTF_CloseFont(fBody);
    TTF_CloseFont(fCaption);
    TTF_CloseFont(fLabel);
    SDL_DestroyRenderer(rnd);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}