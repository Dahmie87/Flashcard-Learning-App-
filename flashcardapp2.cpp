#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// ── Palette ─────────────────────────────────────────────────────────────────
static const SDL_Color COL_BG          = {13,  27,  42,  255}; // #0D1B2A deep navy
static const SDL_Color COL_CARD        = {27,  46,  75,  255}; // #1B2E4B card surface
static const SDL_Color COL_CARD_DARK   = {18,  32,  54,  255}; // slightly darker panel
static const SDL_Color COL_ACCENT      = {78,  205, 196, 255}; // #4ECDC4 teal
static const SDL_Color COL_REVEAL      = {255, 230, 109, 255}; // #FFE66D warm yellow
static const SDL_Color COL_BTN         = {30,  58,  95,  255}; // button base
static const SDL_Color COL_BTN_ACTIVE  = {78,  205, 196, 255}; // teal active
static const SDL_Color COL_WHITE       = {232, 244, 253, 255}; // #E8F4FD off-white
static const SDL_Color COL_MUTED       = {120, 160, 200, 255}; // muted label
static const SDL_Color COL_DOT_OFF     = {40,  65, 100,  255}; // inactive dot
static const SDL_Color COL_SHADOW      = {0,   0,   0,   80};  // subtle shadow

struct FlashCard {
    std::string category;
    std::string question;
    std::string answer;
};

// ── Rounded-rect helper ─────────────────────────────────────────────────────
void drawRoundedRect(SDL_Renderer* r, SDL_Rect rect, int radius,
                     Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    SDL_SetRenderDrawColor(r, red, green, blue, alpha);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Fill main body
    SDL_Rect inner = {rect.x, rect.y + radius, rect.w, rect.h - 2*radius};
    SDL_RenderFillRect(r, &inner);
    SDL_Rect top    = {rect.x + radius, rect.y,             rect.w - 2*radius, radius};
    SDL_Rect bottom = {rect.x + radius, rect.y + rect.h - radius, rect.w - 2*radius, radius};
    SDL_RenderFillRect(r, &top);
    SDL_RenderFillRect(r, &bottom);

    // Approximate corner circles with filled squares for simplicity
    for (int dy = 0; dy < radius; dy++) {
        for (int dx = 0; dx < radius; dx++) {
            float dist = sqrtf((float)((dx-radius)*(dx-radius) + (dy-radius)*(dy-radius)));
            if (dist <= radius) {
                // top-left
                SDL_RenderDrawPoint(r, rect.x + dx, rect.y + dy);
                // top-right
                SDL_RenderDrawPoint(r, rect.x + rect.w - 1 - dx, rect.y + dy);
                // bottom-left
                SDL_RenderDrawPoint(r, rect.x + dx, rect.y + rect.h - 1 - dy);
                // bottom-right
                SDL_RenderDrawPoint(r, rect.x + rect.w - 1 - dx, rect.y + rect.h - 1 - dy);
            }
        }
    }
}

// ── Shadow helper ───────────────────────────────────────────────────────────
void drawShadow(SDL_Renderer* r, SDL_Rect rect, int offset, int radius)
{
    SDL_Rect shadow = {rect.x + offset, rect.y + offset, rect.w, rect.h};
    drawRoundedRect(r, shadow, radius, COL_SHADOW.r, COL_SHADOW.g, COL_SHADOW.b, 90);
}

// ── Accent bar (left border for answer zone) ────────────────────────────────
void drawAccentBar(SDL_Renderer* r, int x, int y, int h, SDL_Color col)
{
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_Rect bar = {x, y, 6, h};
    SDL_RenderFillRect(r, &bar);
}

// ── Progress dots ───────────────────────────────────────────────────────────
void drawProgressDots(SDL_Renderer* r, int total, int current,
                      int centerX, int y, int dotSize, int spacing)
{
    int totalW = total * dotSize + (total-1) * spacing;
    int startX = centerX - totalW / 2;
    for (int i = 0; i < total; i++) {
        int x = startX + i * (dotSize + spacing);
        bool active = (i == current);
        SDL_Color c = active ? COL_ACCENT : COL_DOT_OFF;
        drawRoundedRect(r, {x, y, active ? dotSize+4 : dotSize, dotSize},
                        dotSize/2, c.r, c.g, c.b, 255);
    }
}

// ── Text rendering ──────────────────────────────────────────────────────────
SDL_Texture* makeTextTexture(SDL_Renderer* renderer, TTF_Font* font,
                              const std::string& text, SDL_Color color, int wrapW)
{
    SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, wrapW);
    if (!s) return nullptr;
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    return t;
}

void drawText(SDL_Renderer* renderer, TTF_Font* font,
              const std::string& text, SDL_Color color,
              int x, int y, int wrapW = 700, bool centered = false)
{
    if (text.empty()) return;
    SDL_Texture* tex = makeTextTexture(renderer, font, text, color, wrapW);
    if (!tex) return;
    int w, h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    if (centered) x -= w / 2;
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

// Returns texture height for layout
int drawTextGetH(SDL_Renderer* renderer, TTF_Font* font,
                 const std::string& text, SDL_Color color,
                 int x, int y, int wrapW = 700)
{
    if (text.empty()) return 0;
    SDL_Texture* tex = makeTextTexture(renderer, font, text, color, wrapW);
    if (!tex) return 0;
    int w, h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    return h;
}

bool pointInRect(int x, int y, SDL_Rect r)
{
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// ── Lerp for smooth animation ────────────────────────────────────────────────
float lerp(float a, float b, float t) { return a + (b - a) * t; }

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { std::cout << "SDL failed\n"; return 1; }
    if (TTF_Init() < 0)               { std::cout << "TTF failed\n"; return 1; }

    // Window dimensions — portrait-ish for mobile feel
    const int WIN_W = 540;
    const int WIN_H = 860;

    SDL_Window* window = SDL_CreateWindow(
        "StudyFlash",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Load fonts — try multiple names for cross-platform compat
    TTF_Font* fontLarge  = TTF_OpenFont("DejaVuSans-Bold.ttf", 26);
    if (!fontLarge) fontLarge = TTF_OpenFont("DejaVuSans.ttf", 26);
    TTF_Font* fontMed    = TTF_OpenFont("DejaVuSans.ttf", 20);
    TTF_Font* fontSmall  = TTF_OpenFont("DejaVuSans.ttf", 16);
    TTF_Font* fontLabel  = TTF_OpenFont("DejaVuSans-Bold.ttf", 13);
    if (!fontLabel) fontLabel = TTF_OpenFont("DejaVuSans.ttf", 13);

    if (!fontLarge || !fontMed || !fontSmall || !fontLabel) {
        std::cout << "Font load failed: " << TTF_GetError() << "\n";
        return 1;
    }

    // ── Card data with categories ─────────────────────────────────────────
    std::vector<FlashCard> cards = {
        {"Basics",     "What is C++?",            "A general-purpose, compiled programming language with object-oriented, generic, and low-level memory features."},
        {"Basics",     "What is a Variable?",     "A named storage location in memory that holds a value of a specific type."},
        {"Basics",     "What is a Function?",     "A named, reusable block of code that performs a specific task and can return a value."},
        {"OOP",        "What is OOP?",             "Object-Oriented Programming — a paradigm that organises code into objects combining data and behaviour."},
        {"OOP",        "What is Encapsulation?",  "Bundling data (attributes) and methods that operate on that data together inside a class, hiding internal state."},
        {"OOP",        "What is Inheritance?",    "A mechanism where a derived class acquires properties and behaviours from a base class."},
        {"OOP",        "What is Polymorphism?",   "The ability for different types to be treated through a common interface — one interface, many implementations."},
        {"OOP",        "What is Abstraction?",    "Hiding complex implementation details and exposing only the necessary interface to the user."},
        {"Control",    "What is a Loop?",         "A control-flow statement that repeats a block of code while (or until) a condition holds true."},
        {"Memory",     "What is a Pointer?",      "A variable that stores the memory address of another variable, enabling direct memory manipulation."},
    };

    int currentCard = 0;
    bool showAnswer  = false;
    bool running     = true;

    // ── Animation state ───────────────────────────────────────────────────
    float answerAlpha    = 0.0f;  // 0..255 reveal fade
    float cardOffsetX    = 0.0f;  // slide-in from right
    float targetOffsetX  = 0.0f;
    bool  transitioning  = false;
    int   nextCard       = 0;
    bool  goingRight     = true;

    // ── Layout constants ──────────────────────────────────────────────────
    const int MARGIN     = 24;
    const int CARD_X     = MARGIN;
    const int CARD_Y     = 110;
    const int CARD_W     = WIN_W - 2*MARGIN;
    const int CARD_H     = 490;
    const int CARD_R     = 20;

    // Buttons — three equal, bottom strip
    const int BTN_H      = 58;
    const int BTN_Y      = WIN_H - BTN_H - 24;
    const int BTN_W      = (WIN_W - 2*MARGIN - 16) / 3;
    SDL_Rect btnPrev = {MARGIN,              BTN_Y, BTN_W, BTN_H};
    SDL_Rect btnShow = {MARGIN + BTN_W + 8,  BTN_Y, BTN_W, BTN_H};
    SDL_Rect btnNext = {MARGIN + 2*(BTN_W+8),BTN_Y, BTN_W, BTN_H};

    Uint32 lastTick = SDL_GetTicks();

    while (running)
    {
        Uint32 now   = SDL_GetTicks();
        float  dt    = (now - lastTick) / 1000.0f;
        lastTick     = now;

        // ── Events ───────────────────────────────────────────────────────
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_KEYDOWN && !transitioning)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                        showAnswer = !showAnswer;
                        answerAlpha = showAnswer ? 0.0f : 255.0f;
                        break;
                    case SDLK_RIGHT:
                        nextCard = (currentCard + 1) % (int)cards.size();
                        goingRight = true;
                        transitioning = true;
                        cardOffsetX = 0;
                        targetOffsetX = -(float)WIN_W;
                        showAnswer = false;
                        answerAlpha = 0.0f;
                        break;
                    case SDLK_LEFT:
                        nextCard = currentCard - 1;
                        if (nextCard < 0) nextCard = (int)cards.size()-1;
                        goingRight = false;
                        transitioning = true;
                        cardOffsetX = 0;
                        targetOffsetX = (float)WIN_W;
                        showAnswer = false;
                        answerAlpha = 0.0f;
                        break;
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && !transitioning)
            {
                int mx = event.button.x, my = event.button.y;

                if (pointInRect(mx, my, btnShow))
                {
                    showAnswer = !showAnswer;
                    answerAlpha = showAnswer ? 0.0f : 255.0f;
                }
                if (pointInRect(mx, my, btnNext))
                {
                    nextCard = (currentCard + 1) % (int)cards.size();
                    goingRight = true;
                    transitioning = true;
                    cardOffsetX = 0;
                    targetOffsetX = -(float)WIN_W;
                    showAnswer = false;
                    answerAlpha = 0.0f;
                }
                if (pointInRect(mx, my, btnPrev))
                {
                    nextCard = currentCard - 1;
                    if (nextCard < 0) nextCard = (int)cards.size()-1;
                    goingRight = false;
                    transitioning = true;
                    cardOffsetX = 0;
                    targetOffsetX = (float)WIN_W;
                    showAnswer = false;
                    answerAlpha = 0.0f;
                }
            }
        }

        // ── Animate card slide ────────────────────────────────────────────
        if (transitioning)
        {
            cardOffsetX = lerp(cardOffsetX, targetOffsetX, 12.0f * dt);
            if (fabsf(cardOffsetX - targetOffsetX) < 4.0f)
            {
                cardOffsetX = 0;
                currentCard = nextCard;
                transitioning = false;
            }
        }

        // ── Animate answer fade ───────────────────────────────────────────
        float targetAlpha = showAnswer ? 255.0f : 0.0f;
        answerAlpha = lerp(answerAlpha, targetAlpha, 8.0f * dt);

        // ── Draw ──────────────────────────────────────────────────────────
        SDL_SetRenderDrawColor(renderer, COL_BG.r, COL_BG.g, COL_BG.b, 255);
        SDL_RenderClear(renderer);

        // ── Header: app name + streak ─────────────────────────────────────
        drawText(renderer, fontLabel, "STUDYFLASH",
                 COL_ACCENT, WIN_W/2, 28, 500, true);

        std::string deckLabel = "C++ Fundamentals  ·  " + std::to_string(cards.size()) + " cards";
        drawText(renderer, fontSmall, deckLabel, COL_MUTED,
                 WIN_W/2, 52, 500, true);

        // Divider line under header
        SDL_SetRenderDrawColor(renderer, COL_CARD.r, COL_CARD.g, COL_CARD.b, 255);
        SDL_RenderDrawLine(renderer, MARGIN, 80, WIN_W - MARGIN, 80);

        // ── Progress dots ─────────────────────────────────────────────────
        drawProgressDots(renderer, (int)cards.size(), currentCard,
                         WIN_W/2, 88, 10, 6);

        // ── Card (with slide offset) ──────────────────────────────────────
        int cx = CARD_X + (int)cardOffsetX;

        // Card shadow
        drawShadow(renderer, {cx, CARD_Y, CARD_W, CARD_H}, 6, CARD_R);

        // Card body
        drawRoundedRect(renderer, {cx, CARD_Y, CARD_W, CARD_H},
                        CARD_R, COL_CARD.r, COL_CARD.g, COL_CARD.b, 255);

        // ── Category chip ─────────────────────────────────────────────────
        {
            const std::string& cat = cards[currentCard].category;
            // Measure text width approximately (13px font ≈ 8px/char)
            int chipW = (int)cat.size() * 9 + 24;
            int chipX = cx + 20;
            int chipY = CARD_Y + 20;
            drawRoundedRect(renderer, {chipX, chipY, chipW, 26},
                            13, COL_ACCENT.r/3, COL_ACCENT.g/3, COL_ACCENT.b/3+20, 200);
            drawText(renderer, fontLabel, cat, COL_ACCENT,
                     chipX + 12, chipY + 6, 200);
        }

        // ── Question section ──────────────────────────────────────────────
        int innerX = cx + 28;
        int innerW = CARD_W - 56;

        drawText(renderer, fontLabel, "QUESTION",
                 COL_MUTED, innerX, CARD_Y + 64, innerW);

        // Question text
        drawTextGetH(renderer, fontLarge,
                     cards[currentCard].question,
                     COL_WHITE, innerX, CARD_Y + 90, innerW);

        // ── Separator with "tap to reveal" hint ──────────────────────────
        int sepY = CARD_Y + 260;
        SDL_SetRenderDrawColor(renderer, COL_BTN.r, COL_BTN.g, COL_BTN.b, 180);
        SDL_RenderDrawLine(renderer, cx+20, sepY, cx+CARD_W-20, sepY);

        if (!showAnswer && answerAlpha < 10.0f)
        {
            drawText(renderer, fontSmall, "tap  Reveal  to see the answer",
                     COL_MUTED, WIN_W/2 + (int)cardOffsetX, sepY + 12, 400, true);
        }

        // ── Answer section (fades in) ─────────────────────────────────────
        if (answerAlpha > 2.0f)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            Uint8 a = (Uint8)answerAlpha;

            // Answer panel background
            SDL_SetRenderDrawColor(renderer, COL_CARD_DARK.r, COL_CARD_DARK.g, COL_CARD_DARK.b, a);
            SDL_Rect ansPanel = {cx, sepY + 1, CARD_W, CARD_H - (sepY - CARD_Y) - 1};
            SDL_RenderFillRect(renderer, &ansPanel);

            // Accent bar on left
            SDL_SetRenderDrawColor(renderer, COL_REVEAL.r, COL_REVEAL.g, COL_REVEAL.b, a);
            SDL_Rect abar = {cx, sepY+1, 5, ansPanel.h};
            SDL_RenderFillRect(renderer, &abar);

            // "ANSWER" label
            SDL_Color labelCol = {COL_REVEAL.r, COL_REVEAL.g, COL_REVEAL.b, a};
            drawText(renderer, fontLabel, "ANSWER",
                     labelCol, innerX + 12, sepY + 18, innerW);

            // Answer text
            SDL_Color ansCol = {COL_WHITE.r, COL_WHITE.g, COL_WHITE.b, a};
            drawText(renderer, fontMed,
                     cards[currentCard].answer,
                     ansCol, innerX + 12, sepY + 46, innerW - 12);
        }

        // Card number badge (top-right of card)
        {
            std::string prog = std::to_string(currentCard+1) + "/" + std::to_string(cards.size());
            int bx = cx + CARD_W - 60;
            drawRoundedRect(renderer, {bx, CARD_Y + 18, 50, 28},
                            14, COL_BTN.r, COL_BTN.g, COL_BTN.b, 255);
            drawText(renderer, fontSmall, prog, COL_MUTED, bx + 8, CARD_Y + 24, 60);
        }

        // ── Keyboard hint strip ───────────────────────────────────────────
        int hintY = BTN_Y - 30;
        drawText(renderer, fontSmall,
                 "← →  navigate    SPACE  reveal",
                 COL_DOT_OFF, WIN_W/2, hintY, 400, true);

        // ── Bottom buttons ────────────────────────────────────────────────
        // Prev
        drawShadow(renderer, btnPrev, 3, 14);
        drawRoundedRect(renderer, btnPrev, 14,
                        COL_BTN.r, COL_BTN.g, COL_BTN.b, 255);
        drawText(renderer, fontMed, "< Prev", COL_MUTED,
                 btnPrev.x + btnPrev.w/2, btnPrev.y + 16, btnPrev.w, true);

        // Show/Reveal — accent colour, stands out
        drawShadow(renderer, btnShow, 3, 14);
        SDL_Color showBtnCol = showAnswer ? COL_REVEAL : COL_BTN_ACTIVE;
        drawRoundedRect(renderer, btnShow, 14,
                        showBtnCol.r, showBtnCol.g, showBtnCol.b, 255);
        {
            SDL_Color lbl = {COL_BG.r, COL_BG.g, COL_BG.b, 255};
            const char* showLbl = showAnswer ? "Hide" : "Reveal";
            drawText(renderer, fontMed, showLbl, lbl,
                     btnShow.x + btnShow.w/2, btnShow.y + 16, btnShow.w, true);
        }

        // Next
        drawShadow(renderer, btnNext, 3, 14);
        drawRoundedRect(renderer, btnNext, 14,
                        COL_BTN.r, COL_BTN.g, COL_BTN.b, 255);
        drawText(renderer, fontMed, "Next >", COL_MUTED,
                 btnNext.x + btnNext.w/2, btnNext.y + 16, btnNext.w, true);

        SDL_RenderPresent(renderer);
        SDL_Delay(14);
    }

    TTF_CloseFont(fontLarge);
    TTF_CloseFont(fontMed);
    TTF_CloseFont(fontSmall);
    TTF_CloseFont(fontLabel);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
