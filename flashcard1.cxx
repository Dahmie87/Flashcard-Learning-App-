#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>

struct FlashCard {
    std::string question;
    std::string answer;
};

SDL_Texture* renderText(
    const std::string& text,
    SDL_Color color,
    TTF_Font* font,
    SDL_Renderer* renderer)
{
    SDL_Surface* surface =
        TTF_RenderUTF8_Blended_Wrapped(
            font,
            text.c_str(),
            color,
            700);

    if (!surface)
        return nullptr;

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);
    return texture;
}

void drawText(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const std::string& text,
    int x,
    int y)
{
    SDL_Color color = {255,255,255,255};

    SDL_Texture* texture =
        renderText(text, color, font, renderer);

    if (!texture)
        return;

    int w,h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    SDL_Rect dst = {x,y,w,h};

    SDL_RenderCopy(renderer, texture, NULL, &dst);

    SDL_DestroyTexture(texture);
}

bool pointInRect(int x,int y, SDL_Rect r)
{
    return x >= r.x &&
           x <= r.x + r.w &&
           y >= r.y &&
           y <= r.y + r.h;
}

int main()
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout<<"SDL failed\n";
        return 1;
    }

    if(TTF_Init() < 0)
    {
        std::cout<<"TTF failed\n";
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow(
            "Flash Card Learning App",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            900,
            700,
            SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer =
        SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED);

    TTF_Font* font =
        TTF_OpenFont("DejaVuSans.ttf", 28);

    if(!font)
    {
        std::cout<<"Font not found.\n";
        return 1;
    }

    std::vector<FlashCard> cards =
    {
        {"What is C++?",
         "A general-purpose programming language."},

        {"What is a Variable?",
         "A named storage location in memory."},

        {"What is a Function?",
         "A reusable block of code."},

        {"What is OOP?",
         "Object Oriented Programming."},

        {"What is Encapsulation?",
         "Bundling data and methods together."},

        {"What is Inheritance?",
         "Creating a class from another class."},

        {"What is Polymorphism?",
         "One interface, many forms."},

        {"What is Abstraction?",
         "Hiding implementation details."},

        {"What is a Loop?",
         "Repeats code multiple times."},

        {"What is a Pointer?",
         "Stores a memory address."}
    };

    int currentCard = 0;
    bool showAnswer = false;
    bool running = true;

    SDL_Rect btnShow =
    {
        100,580,180,60
    };

    SDL_Rect btnPrev =
    {
        330,580,180,60
    };

    SDL_Rect btnNext =
    {
        560,580,180,60
    };

    while(running)
    {
        SDL_Event event;

        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
            }

            if(event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                        showAnswer = !showAnswer;
                        break;

                    case SDLK_RIGHT:
                        currentCard =
                            (currentCard + 1)
                            % cards.size();

                        showAnswer = false;
                        break;

                    case SDLK_LEFT:
                        currentCard--;

                        if(currentCard < 0)
                            currentCard =
                                cards.size()-1;

                        showAnswer = false;
                        break;
                }
            }

            if(event.type == SDL_MOUSEBUTTONDOWN)
            {
                int mx =
                    event.button.x;

                int my =
                    event.button.y;

                if(pointInRect(mx,my,btnShow))
                {
                    showAnswer =
                        !showAnswer;
                }

                if(pointInRect(mx,my,btnNext))
                {
                    currentCard =
                        (currentCard + 1)
                        % cards.size();

                    showAnswer = false;
                }

                if(pointInRect(mx,my,btnPrev))
                {
                    currentCard--;

                    if(currentCard < 0)
                        currentCard =
                            cards.size()-1;

                    showAnswer = false;
                }
            }
        }

        SDL_SetRenderDrawColor(
            renderer,
            20,20,30,255);

        SDL_RenderClear(renderer);

        SDL_Rect cardArea =
        {
            50,
            50,
            800,
            450
        };

        SDL_SetRenderDrawColor(
            renderer,
            50,80,150,255);

        SDL_RenderFillRect(
            renderer,
            &cardArea);

        drawText(
            renderer,
            font,
            "FLASHCARD LEARNING APP",
            240,
            10);

        std::string progress =
            "Card "
            + std::to_string(currentCard+1)
            + " / "
            + std::to_string(cards.size());

        drawText(
            renderer,
            font,
            progress,
            350,
            80);

        drawText(
            renderer,
            font,
            "QUESTION:",
            90,
            150);

        drawText(
            renderer,
            font,
            cards[currentCard].question,
            90,
            210);

        if(showAnswer)
        {
            drawText(
                renderer,
                font,
                "ANSWER:",
                90,
                320);

            drawText(
                renderer,
                font,
                cards[currentCard].answer,
                90,
                380);
        }

        SDL_SetRenderDrawColor(
            renderer,
            0,140,220,255);

        SDL_RenderFillRect(
            renderer,
            &btnShow);

        SDL_RenderFillRect(
            renderer,
            &btnPrev);

        SDL_RenderFillRect(
            renderer,
            &btnNext);

        drawText(
            renderer,
            font,
            "Show",
            140,
            595);

        drawText(
            renderer,
            font,
            "Prev",
            380,
            595);

        drawText(
            renderer,
            font,
            "Next",
            620,
            595);

        drawText(
            renderer,
            font,
            "SPACE = Show/Hide    LEFT = Previous    RIGHT = Next",
            100,
            660);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();

    return 0;
}