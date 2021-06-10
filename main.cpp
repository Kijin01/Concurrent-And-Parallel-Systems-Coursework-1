////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <math.h>
#include <array>
#include <mutex>
#include <chrono>

//stb library 
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <iostream>


namespace fs = std::filesystem;
std::atomic<bool> window_is_open = false;
std::atomic<bool> loading_done = true;

std::mutex mut;

// Define some constants
const float pi = 3.14159f;
const int gameWidth = 800;
const int gameHeight = 600;



//Stuff for the rgb to hsv conversion. I just took this example from https://www.tutorialspoint.com/c-program-to-change-rgb-color-model-to-hsv-color-model
float max(float a, float b, float c) {
    return ((a > b) ? (a > c ? a : c) : (b > c ? b : c));
}
float min(float a, float b, float c) {
    return ((a < b) ? (a < c ? a : c) : (b < c ? b : c));
}
float rgb_to_hsv(float r, float g, float b) {
    // R, G, B values are divided by 255
    // to change the range from 0..255 to 0..1:

    float h, s, v;
    r /= 255.0;
    g /= 255.0;
    b /= 255.0;
    float cmax = max(r, g, b); // maximum of r, g, b
    float cmin = min(r, g, b); // minimum of r, g, b
    float diff = cmax - cmin; // diff of cmax and cmin.
    if (cmax == cmin)
        h = 0;
    else if (cmax == r)
        h = fmod((60 * ((g - b) / diff) + 360), 360.0);
    else if (cmax == g)
        h = fmod((60 * ((b - r) / diff) + 120), 360.0);
    else if (cmax == b)
        h = fmod((60 * ((r - g) / diff) + 240), 360.0);
    // if cmax equal zero
    if (cmax == 0)
        s = 0;
    else
        s = (diff / cmax) * 100;
    // compute v
    v = cmax * 100;
    // return the h (hue) value, since it is the only one we care about
    return h;

}


std::map<float, std::string> map;   //map for all the image filenames with their respective median hue
std::vector<float> normalized_precise_median_hues;

std::vector<std::string> imageFilenames;


void load_images_stb_median_hue(const char* filename) {
    // Load image using stb library
        
    
        int width, height, channels;
        unsigned char* img = stbi_load(filename, &width, &height, &channels, 3);
        if (img == NULL) {
            printf("Error loading image\n");
            exit(1);
        }
        
        printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

        //get the size of the image
        int img_size = width * height * channels;
        //loop through rgb triplets and convert them to HSV on the spot
        std::vector<float> hues;
        for (int i = 0; i <= img_size; i += 3) {
            float h;
            float r = (float_t)(img[i]);
            float g = (float_t)(img[i + 1]);
            float b = (float_t)(img[i + 2]);
            
            //once we have our rgb triplet, we convert it straight to hsv
            h = rgb_to_hsv(r, g, b); //we only care about the hue value
            hues.push_back(h);
        }

        //Once we have all the hues for the picture, we sort them
        std::sort(hues.begin(), hues.end());
        float median_hue = hues.at((hues.size() / 2));
        float remainder = median_hue - trunc(median_hue); //we do this in case if there are duplicate normalized values
        int normalized_hue = ((int)trunc((median_hue + 60.0f))) % 360;
        float precise_normalized_hue = normalized_hue + remainder;

        printf("median hue: %f\n", median_hue);
        printf("normalized hue: %d\n", normalized_hue);
        printf("remainder: %f\n", remainder);
        printf("precise normalized hue: %f\n", precise_normalized_hue);
        map[precise_normalized_hue] = (std::string)filename; //link the precise float hues to filenames
        normalized_precise_median_hues.push_back(precise_normalized_hue);
        //free memory once we are done getting the data from the picture        
        stbi_image_free(img);
}


void load_images(std::vector<std::string> &filenames) {
    for (int i = 0; i < filenames.size(); i++) {
        load_images_stb_median_hue(filenames[i].c_str());
        std::cout << filenames[i].c_str() << "\n\n\n";
    }   
}

void run_GUI(int index, const int width, const int height, sf::RenderWindow &window, sf::Sprite &sprite, sf::Texture &texture) {

    while (window.isOpen()) {
        // Handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Window closed or escape key pressed: exit
            if ((event.type == sf::Event::Closed) ||
                ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape)))
            {
                window.close();
                window_is_open = false;
                break;
            }

            // Window size changed, adjust view appropriately
            if (event.type == sf::Event::Resized)
            {
                sf::View view;
                view.setSize(width, height);
                view.setCenter(width / 2.f, height / 2.f);
                window.setView(view);
            }

            // Arrow key handling!
            if (event.type == sf::Event::KeyPressed)
            {

                if (!((index + 1) > imageFilenames.size()) && loading_done == true) {
                    // adjust the image index
                    if (event.key.code == sf::Keyboard::Key::Left) {
                        index = (index + normalized_precise_median_hues.size() - 1) % normalized_precise_median_hues.size();
                    }
                    else if (event.key.code == sf::Keyboard::Key::Right) {
                        index = (index + 1) % normalized_precise_median_hues.size();
                    }
                    // ... and load the appropriate texture, and put it in the sprite
                    if (texture.loadFromFile(map[normalized_precise_median_hues[index]]))
                    {
                        sprite = sf::Sprite(texture);
                        sprite.setScale(0.4, 0.4);
                        //sprite.setScale(gameWidth / sprite.getTexture()->getSize().x, gameHeight / sprite.getTexture()->getSize().y);
                        window.setTitle(map[normalized_precise_median_hues[index]]);


                    }
                }
                else {
                    std::cout << "image not yet loaded!" << std::endl;
                    texture.loadFromFile("loading.jpg");
                    sprite = sf::Sprite(texture);
                    sprite.setScale(0.4, 0.4);
                }
                std::cout << index << " is the current index.\n";
                std::cout << "X: " << sprite.getTexture()->getSize().x << "Y: " << sprite.getTexture()->getSize().y << std::endl;
            }
        }

        // Clear the window
        window.clear(sf::Color(50, 200, 50));
        // draw the sprite
        window.draw(sprite);
        // Display things on screen
        window.display();

    }   
    
}

void sort_hues() {
    std::sort(normalized_precise_median_hues.begin(), normalized_precise_median_hues.end());
}

bool need_reload_images(const std::string& folder) {
    std::lock_guard<std::mutex> lock(mut);

    std::vector<std::string> test;
    for (auto& p : fs::directory_iterator(folder)) {
        test.push_back(p.path().string());
    }
    
    if (test != imageFilenames) {
        imageFilenames = test;
        return true;
    }
    else {
        return false;
    }
}

void background_loading_sorting(const std::string &folder) {
    while (window_is_open == true) {
        if (need_reload_images(folder) == true) {
            std::lock_guard<std::mutex> lock(mut);
            loading_done = false;
            std::cout << std::this_thread::get_id() << " thread id, background_loading_sorting started\n" << std::endl;
            if (!imageFilenames.empty()) {
                map.clear();
                normalized_precise_median_hues.clear();
                load_images(imageFilenames);
                sort_hues();
            }
            std::cout << std::this_thread::get_id() << " thread id, background_loading_sorting finished\n" << std::endl;
            loading_done = true;

        }       
    }  
}


int main(int argc, char **argv)
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    //Folder to load images
    //constexpr char* image_folder = "D:/unsorted";
    const std::string string_folder = (std::string)argv[1];
   
    for (auto& p : fs::directory_iterator(string_folder)) {
        imageFilenames.push_back(p.path().u8string());

    }

    int imageIndex = 0;

    //Load all the stuff initially
    load_images(imageFilenames);
    sort_hues();

    // Create the window of the application
    sf::RenderWindow window(sf::VideoMode(gameWidth, gameHeight, 32), "Image Fever",
        sf::Style::Titlebar | sf::Style::Close);
    window.setVerticalSyncEnabled(true);
    window_is_open = true;

    //start the loading and sorting thread, will always be active but only will execute if imageFilenames is changed and requires a reload
    std::thread a(background_loading_sorting, string_folder);

    //DEBUG
    unsigned int max_threads = std::thread::hardware_concurrency();
    std::cout << max_threads << " concurrent threads are supported.\n\n\n";

    // Load an image to begin with
    sf::Texture texture;
    
    if (!texture.loadFromFile(map[normalized_precise_median_hues[imageIndex]])) 
        return EXIT_FAILURE;
    
       
    sf::Sprite sprite(texture);
    sprite.setScale(0.4, 0.4);
    //sprite.setScale(gameWidth/sprite.getTexture()->getSize().x, gameHeight / sprite.getTexture()->getSize().y);
    window.setTitle(map[normalized_precise_median_hues[imageIndex]]);
    std::cout << imageIndex << " is the current index.\n";
    std::cout << "X:" << sprite.getTexture()->getSize().x << " Y:" << sprite.getTexture()->getSize().y << std::endl;

    //run GUI on main thread
    sf::Clock clock;
    
    run_GUI(imageIndex, gameWidth, gameHeight, window, sprite, texture);

    if (!window.isOpen()) {
        a.join();
    }

    return EXIT_SUCCESS;
}
