#include "../Header/Util.h"

#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "../Header/stb_image.h"

// Autor: Nedeljko Tesanovic
// Opis: pomocne funkcije za zaustavljanje programa, ucitavanje sejdera, tekstura i kursora
// Smeju se koristiti tokom izrade projekta

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla; ako nije moguće otvoriti tačnu putanju, pokušaj sa prefiksima (../, ../../)
    std::string content = "";
    std::vector<std::string> prefixes = {"", "../", "../../"};
    for (const auto& p : prefixes)
    {
        std::ifstream file(p + std::string(source));
        if (file.is_open())
        {
            std::stringstream ss;
            ss << file.rdbuf();
            content = ss.str();
            file.close();
            std::cout << "Uspjesno procitao fajl sa putanje \"" << p << source << "\"!" << std::endl;
            break;
        }
    }
    if (content.empty())
    {
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"! Pokušano je sa ../ i ../../ prefiksima." << std::endl;
    }
    std::string temp = content;
    const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)

    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf("%s", infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    // Do not call glValidateProgram here; validation may require a bound VAO and causes "No vertex array object bound." messages

    int linkStatus = GL_FALSE;
    char infoLog[1024] = {0};
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
    if (linkStatus == GL_FALSE)
    {
        std::cout << "Objedinjeni sejder link failed! Greska: \n";
        std::cout << infoLog << std::endl;
    }
    else if (infoLog[0] != '\0')
    {
        std::cout << "Objedinjeni sejder log: \n" << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

GLFWcursor* loadImageToCursor(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;

    // glfwCreateCursor ocekuje RGBA (4 kanala), pa forsiramo konverziju
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, STBI_rgb_alpha);

    if (ImageData != NULL)
    {
        GLFWimage image;
        image.width = TextureWidth;
        image.height = TextureHeight;
        image.pixels = ImageData;

        // Tacka na povrsini slike kursora koja se ponasa kao hitboks (laserska lampica daljinskog)
        int hotspotX = 0;
        int hotspotY = 0;

        GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
        stbi_image_free(ImageData);
        return cursor;
    }
    else {
        std::cout << "Kursor nije ucitan! Putanja kursora: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return nullptr;
    }
}

GLFWcursor* createProceduralRemoteCursor(int width, int height)
{
    if (width <= 0 || height <= 0) return nullptr;

    // Transparent background with simple remote body and a small laser dot at top-left
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);

    const unsigned char body[4] = { 210, 215, 223, 255 };
    const unsigned char edge[4] = { 80, 80, 90, 255 };
    const unsigned char laser[4] = { 235, 64, 52, 255 };

    auto put = [&](int x, int y, const unsigned char color[4])
    {
        if (x < 0 || y < 0 || x >= width || y >= height) return;
        size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 4;
        pixels[idx + 0] = color[0];
        pixels[idx + 1] = color[1];
        pixels[idx + 2] = color[2];
        pixels[idx + 3] = color[3];
    };

    int bodyX = 8;
    int bodyY = 8;
    int bodyW = width - 16;
    int bodyH = height - 16;

    for (int y = bodyY; y < bodyY + bodyH; ++y)
    {
        for (int x = bodyX; x < bodyX + bodyW; ++x)
        {
            put(x, y, body);
        }
    }

    for (int y = bodyY; y < bodyY + bodyH; ++y)
    {
        put(bodyX, y, edge);
        put(bodyX + bodyW - 1, y, edge);
    }
    for (int x = bodyX; x < bodyX + bodyW; ++x)
    {
        put(x, bodyY, edge);
        put(x, bodyY + bodyH - 1, edge);
    }

    int r = 5;
    int cx = r;
    int cy = r;
    for (int y = -r; y <= r; ++y)
    {
        for (int x = -r; x <= r; ++x)
        {
            if (x * x + y * y <= r * r)
            {
                put(cx + x, cy + y, laser);
            }
        }
    }

    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = pixels.data();

    int hotspotX = 0;
    int hotspotY = 0;
    return glfwCreateCursor(&image, hotspotX, hotspotY);
}
