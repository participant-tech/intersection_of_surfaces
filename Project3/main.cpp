#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <vector>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

// сфера
const double sphereRadius = 1.5;

// эллипс
const double ellA = 1.75;
const double ellB = 1.6;
const double ellC = 1.2;

// количество плоскостей
const int numPlanes = 300;

struct Point {
    double x, y, z;
};

HWND g_hWnd;
HDC g_hDC;
HGLRC g_hRC;
int g_width = 800;
int g_height = 600;
float g_angleX = 30.0f;
float g_angleY = 45.0f;
int g_lastMouseX = 0;
int g_lastMouseY = 0;
bool g_mouseDown = false;

std::vector<Point> g_intersectionPoints;

// находит точки пересечения одной плоскостью
std::vector<Point> Find_points_intersection(double x) {
    std::vector<Point> points;

    double sphereY2Z2 = sphereRadius * sphereRadius - x * x;
    if (sphereY2Z2 < 0) return points;

    double ellPart = 1.0 - (x * x) / (ellA * ellA);
    if (ellPart < 0) return points;

    double invB2 = 1.0 / (ellB * ellB);
    double invC2 = 1.0 / (ellC * ellC);

    double coeffZ2 = invC2 - invB2;
    double rightSide = ellPart - sphereY2Z2 * invB2;

    if (fabs(coeffZ2) < 1e-8) {
        if (sphereY2Z2 >= 0 && ellPart >= 0) {
            double rSq = sphereY2Z2;
            int localSegments = 50;
            double angleStep = 2 * 3.14159265358979323846 / localSegments;
            for (int i = 0; i <= localSegments; i++) {
                double angle = i * angleStep;
                double y = sqrt(rSq) * cos(angle);
                double z = sqrt(rSq) * sin(angle);
                points.push_back({ x, y, z });
            }
        }
        return points;
    }

    double z2 = rightSide / coeffZ2;
    if (z2 < 0 || z2 > sphereY2Z2) return points;

    double z = sqrt(z2);
    double y2 = sphereY2Z2 - z2;
    if (y2 < 0) return points;

    double y = sqrt(y2);

    points.push_back({ x,  y,  z });
    points.push_back({ x,  y, -z });
    points.push_back({ x, -y,  z });
    points.push_back({ x, -y, -z });

    return points;
}

// находит линию
std::vector<Point> Find_intersection_line() {
    std::vector<Point> allPoints;

    double xMinSphere = -sphereRadius;
    double xMaxSphere = sphereRadius;
    double xMinEll = -ellA;
    double xMaxEll = ellA;

    double xMin = (xMinSphere > xMinEll) ? xMinSphere : xMinEll;
    double xMax = (xMaxSphere < xMaxEll) ? xMaxSphere : xMaxEll;

    double step = (xMax - xMin) / numPlanes;

    for (int i = 0; i <= numPlanes; i++) {       // проводим множество плоскостей
        double x = xMin + i * step;
        auto points = Find_points_intersection(x);
        allPoints.insert(allPoints.end(), points.begin(), points.end());
    }

    return allPoints;
}

// рисует линию
void Draw_intersection_line() {
    if (g_intersectionPoints.empty()) return;

    glDisable(GL_LIGHTING);

    glColor3f(1.0f, 1.0f, 0.0f);
    glLineWidth(4.0f);

    std::vector<Point> sorted;
    for (double x = -2.5; x <= 2.5; x += 0.03) {
        for (size_t i = 0; i < g_intersectionPoints.size(); i++) {
            const Point& p = g_intersectionPoints[i];
            double dx = p.x - x;
            if (dx < 0) dx = -dx;
            if (dx < 0.02) {
                sorted.push_back(p);
            }
        }
    }

    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < sorted.size(); i++) {
        glVertex3d(sorted[i].x, sorted[i].y, sorted[i].z);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

void Draw_sphere() {
    GLfloat sphereMaterial[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphereMaterial);
    glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);

    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluSphere(quad, sphereRadius, 100, 100);
    gluDeleteQuadric(quad);
}

void Draw_ellipsoid() {
    GLfloat ellMaterial[] = { 0.85f, 0.65f, 0.15f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ellMaterial);
    glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

    glPushMatrix();
    glScalef(ellA, ellB, ellC);
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluSphere(quad, 1.0, 100, 100);
    gluDeleteQuadric(quad);
    glPopMatrix();
}

void Draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(4.0, 3.0, 4.0,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0);

    glRotatef(g_angleX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_angleY, 0.0f, 1.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    Draw_ellipsoid();
    Draw_sphere();
    Draw_intersection_line();

    SwapBuffers(g_hDC);
}

void SetupOpenGL() {
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 3.0f, 4.0f, 5.0f, 1.0f };
    GLfloat lightAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat lightDiffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    GLfloat lightSpecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    GLfloat globalAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)g_width / g_height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message) {

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        Draw();
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        g_width = LOWORD(lParam);
        g_height = HIWORD(lParam);

        if (g_hRC) {
            glViewport(0, 0, g_width, g_height);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(45.0, (double)g_width / g_height, 0.1, 100.0);
            glMatrixMode(GL_MODELVIEW);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (g_mouseDown) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            int dx = x - g_lastMouseX;
            int dy = y - g_lastMouseY;

            g_angleY += dx * 0.5f;
            g_angleX += dy * 0.5f;

            if (g_angleX > 90.0f) g_angleX = 90.0f;
            if (g_angleX < -90.0f) g_angleX = -90.0f;

            g_lastMouseX = x;
            g_lastMouseY = y;

            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        g_mouseDown = true;
        g_lastMouseX = LOWORD(lParam);
        g_lastMouseY = HIWORD(lParam);
        SetCapture(hWnd);
        return 0;
    }

    case WM_LBUTTONUP: {
        g_mouseDown = false;
        ReleaseCapture();
        return 0;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        return 0;
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

bool createOpenGLWindow(HWND hWnd) {
    g_hDC = GetDC(hWnd);
    if (!g_hDC) return false;

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(g_hDC, &pfd);
    if (!pixelFormat) return false;

    if (!SetPixelFormat(g_hDC, pixelFormat, &pfd)) return false;

    g_hRC = wglCreateContext(g_hDC);
    if (!g_hRC) return false;

    if (!wglMakeCurrent(g_hDC, g_hRC)) return false;

    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASS wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"OpenGLWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Failed to register window class", L"Error", MB_OK);
        return 1;
    }

    g_hWnd = CreateWindow(
        L"OpenGLWindow",
        L"Пересечение сферы и эллипсоида",
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        g_width, g_height,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return 1;

    if (!createOpenGLWindow(g_hWnd)) {
        MessageBox(NULL, L"Failed to create OpenGL context", L"Error", MB_OK);
        return 1;
    }

    SetupOpenGL();
    // находим линию пересечения
    g_intersectionPoints = Find_intersection_line();

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
     
    InvalidateRect(g_hWnd, NULL, TRUE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hRC);
    ReleaseDC(g_hWnd, g_hDC);

    return msg.wParam;
}