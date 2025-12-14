/**
 * Clay Bar Chart Extension - Demo
 * 
 * This file demonstrates how to use the bar chart extension with Clay and Raylib.
 * It shows both vertical and horizontal chart orientations with sample data.
 */

#define CLAY_IMPLEMENTATION
#include "../../clay.h"
#include "../../renderers/raylib/clay_renderer_raylib.c"

// Define which extensions to load
#define CLAY_USE_BARCHART
#define CLAY_USE_PIECHART
#include "extensions/extensions.h"

#include <stdlib.h>
#include <time.h>

// Font IDs
const uint32_t FONT_ID_BODY_16 = 0;

// Colors
#define COLOR_BLUE (Clay_Color) {100, 150, 250, 255}
#define COLOR_GREEN (Clay_Color) {120, 200, 120, 255}
#define COLOR_ORANGE (Clay_Color) {250, 180, 100, 255}
#define COLOR_RED (Clay_Color) {250, 100, 100, 255}
#define COLOR_PURPLE (Clay_Color) {180, 120, 250, 255}
#define COLOR_TEAL (Clay_Color) {100, 200, 200, 255}
#define COLOR_YELLOW (Clay_Color) {250, 250, 100, 255}
#define COLOR_PINK (Clay_Color) {250, 100, 250, 255}
#define COLOR_BROWN (Clay_Color) {150, 100, 50, 255}
#define COLOR_GRAY (Clay_Color) {150, 150, 150, 255}
#define COLOR_CYAN (Clay_Color) {100, 250, 250, 255}
#define COLOR_LIME (Clay_Color) {150, 250, 100, 255}
#define COLOR_BLACK (Clay_Color) {0, 0, 0, 255}


#define len(arr) (sizeof(arr) / sizeof((arr)[0]))
#define SALES_DATA_COUNT 12

// Month names for rotating labels
static const char* monthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// Dynamic sales data that updates every second
static Clay_BarChart_DataPoint salesData[SALES_DATA_COUNT];
static Clay_PieChart_DataPoint pieData[SALES_DATA_COUNT];  // Pie chart version of sales data
static Clay_PieChart_Config pieConfig;  // Store pie config for drawing
static int currentMonthIndex = 0;
static double lastUpdateTime = 0.0;

// Color mode for pie chart (can be changed with keyboard)
static Clay_PieChart_ColorMode pieChartColorMode = CLAY_PIECHART_COLOR_MODE_GRADIENT;

// Define color palette for pie chart
static Clay_Color pieChartPalette[] = {
    COLOR_BLUE, COLOR_GREEN, COLOR_ORANGE, COLOR_RED, COLOR_PURPLE, COLOR_TEAL
};

// Initialize sales data with starting values
void InitializeSalesData(void) {
    static float baseValues[] = {
        125.5f, 142.0f, 138.3f, 165.7f, 158.2f, 175.9f,
        168.4f, 182.1f, 171.5f, 195.2f, 188.7f, 203.4f
    };
    
    for (int i = 0; i < SALES_DATA_COUNT; i++) {
        salesData[i].value = baseValues[i];
        salesData[i].label = (Clay_String) {
            .isStaticallyAllocated = true,
            .length = 3,
            .chars = monthNames[i]
        };
        // Alternate colors
        salesData[i].color = (i % 2) ? COLOR_BLUE : COLOR_GREEN;
        
        // Initialize pie data
        pieData[i].value = baseValues[i];
        pieData[i].label = salesData[i].label;
        pieData[i].color = salesData[i].color;
        pieData[i].exploded = false;
    }
    
    currentMonthIndex = 0;
    lastUpdateTime = GetTime();
}

// Update sales data - shift left and add new random value
void UpdateSalesData(void) {
    double currentTime = GetTime();
    
    // Update every second
    if (currentTime - lastUpdateTime >= 1.0) {
        // Shift all values to the left (remove first month)
        for (int i = 0; i < SALES_DATA_COUNT - 1; i++) {
            salesData[i].value = salesData[i + 1].value;
        }
        
        // Add new random value at the end (100-250 range)
        salesData[SALES_DATA_COUNT - 1].value = 100.0f + (float)(rand() % 151);
        
        // Update month labels to rotate
        currentMonthIndex = (currentMonthIndex + 1) % 12;
        for (int i = 0; i < SALES_DATA_COUNT; i++) {
            int monthIdx = (currentMonthIndex + i) % 12;
            salesData[i].label = (Clay_String) {
                .isStaticallyAllocated = true,
                .length = 3,
                .chars = monthNames[monthIdx]
            };
            // Update color based on position
            switch(i)
            {
                case 0:  salesData[i].color = COLOR_BLUE; break;
                case 1:  salesData[i].color = COLOR_GREEN; break;
                case 2:  salesData[i].color = COLOR_ORANGE; break;
                case 3:  salesData[i].color = COLOR_RED; break;
                case 4:  salesData[i].color = COLOR_PURPLE; break;
                case 5:  salesData[i].color = COLOR_TEAL; break;
                case 6:  salesData[i].color = COLOR_YELLOW; break;
                case 7:  salesData[i].color = COLOR_PINK; break;
                case 8:  salesData[i].color = COLOR_BROWN; break;
                case 9:  salesData[i].color = COLOR_GRAY; break;
                case 10: salesData[i].color = COLOR_CYAN; break;
                case 11: salesData[i].color = COLOR_LIME; break;
                default: salesData[i].color = COLOR_BLACK; break;
            };
        }
        
        // Update pie data to match all sales data values
        for (int i = 0; i < SALES_DATA_COUNT; i++) {
            pieData[i].value = salesData[i].value;
            pieData[i].label = salesData[i].label;
            pieData[i].color = salesData[i].color;
        }
        
        lastUpdateTime = currentTime;
    }
}

// Create the layout with bar charts
Clay_RenderCommandArray CreateLayout(void) {
    Clay_BeginLayout();
    
    // Main container
    CLAY(
        CLAY_ID("OuterContainer"), 
        {
            .layout = {
                .sizing = { 
                    .width = CLAY_SIZING_GROW(0), 
                    .height = CLAY_SIZING_GROW(0) 
                },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = CLAY_PADDING_ALL(32),
                .childGap = 32
            },
            .backgroundColor = {230, 230, 235, 255}
        }
    ) {
        // Title
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { .width = CLAY_SIZING_GROW(0) },
                .padding = { 0, 0, 0, 16 }
            }
        }) {
            CLAY_TEXT(
                CLAY_STRING("Clay Dashboard - Bar Chart & Pie Chart"),
                CLAY_TEXT_CONFIG({
                    .fontSize = 32,
                    .fontId = FONT_ID_BODY_16,
                    .textColor = {40, 40, 50, 255}
                })
            );
        }
        
        // Charts container
        CLAY(
            CLAY_ID("ChartsContainer"),
            {
                .layout = {
                    .sizing = { 
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0)
                    },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childGap = 24
                }
            }
        ) {
            // Left panel - Vertical chart
            CLAY(
                CLAY_ID("LeftPanel"),
                {
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .sizing = { 
                            .width = CLAY_SIZING_PERCENT(0.5),
                            .height = CLAY_SIZING_GROW(0)
                        },
                        .childGap = 16
                    }
                }
            ) {
                // Chart title
                CLAY_AUTO_ID({
                    .layout = { 
                        .sizing = { .width = CLAY_SIZING_GROW(0) },
                        .padding = { 8, 8, 8, 8 }
                    },
                    .backgroundColor = {255, 255, 255, 255},
                    .cornerRadius = CLAY_CORNER_RADIUS(8)
                }) {
                    CLAY_TEXT(
                        CLAY_STRING("Monthly Sales (Thousands)"),
                        CLAY_TEXT_CONFIG({
                            .fontSize = 20,
                            .fontId = FONT_ID_BODY_16,
                            .textColor = {60, 60, 70, 255}
                        })
                    );
                }
                
                // Vertical bar chart
                Clay_BarChart_Config verticalConfig = Clay_BarChart_DefaultConfig();
                verticalConfig.data = salesData;
                verticalConfig.dataCount = SALES_DATA_COUNT;
                verticalConfig.orientation = CLAY_BARCHART_ORIENTATION_VERTICAL;
                verticalConfig.barWidth = 60.0f;
                verticalConfig.barGap = 12.0f;
                verticalConfig.showLabels = true;
                verticalConfig.showValues = true;
                verticalConfig.backgroundColor = (Clay_Color){255, 255, 255, 255};
                verticalConfig.labelFontId = FONT_ID_BODY_16;
                // Use gradient color mode (blue to green)
                verticalConfig.colorMode = CLAY_BARCHART_COLOR_MODE_GRADIENT;
                verticalConfig.colorConfig.gradient = (Clay_BarChart_Gradient){
                    .start = COLOR_BLUE,
                    .end = COLOR_GREEN
                };
                
                // Calculate width based on number of bars
                float chartWidth = (verticalConfig.barWidth + verticalConfig.barGap) * verticalConfig.dataCount + 32;
                
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_GROW(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
                    }
                }) {
                    Clay_BarChart_Render(CLAY_STRING("SalesChart"), &verticalConfig, chartWidth, 400);
                }
            }
            
            // Right panel - Pie chart
            CLAY(
                CLAY_ID("RightPanel"),
                {
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .sizing = { 
                            .width = CLAY_SIZING_PERCENT(0.5),
                            .height = CLAY_SIZING_GROW(0)
                        },
                        .childGap = 16
                    }
                }
            ) {
                // Chart title
                CLAY_AUTO_ID({
                    .layout = { 
                        .sizing = { .width = CLAY_SIZING_GROW(0) },
                        .padding = { 8, 8, 8, 8 }
                    },
                    .backgroundColor = {255, 255, 255, 255},
                    .cornerRadius = CLAY_CORNER_RADIUS(8)
                }) {
                    CLAY_TEXT(
                        CLAY_STRING("Sales Distribution"),
                        CLAY_TEXT_CONFIG({
                            .fontSize = 20,
                            .fontId = FONT_ID_BODY_16,
                            .textColor = {60, 60, 70, 255}
                        })
                    );
                }
                
                // Pie chart using sales data
                pieConfig = Clay_PieChart_DefaultConfig();
                pieConfig.data = pieData;
                pieConfig.dataCount = SALES_DATA_COUNT;
                pieConfig.radius = 120.0f;
                pieConfig.donutHoleRadius = 50.0f;  // Donut chart
                pieConfig.showLegend = true;
                pieConfig.showPercentages = true;
                pieConfig.showValues = false;
                pieConfig.backgroundColor = (Clay_Color){255, 255, 255, 255};
                pieConfig.labelFontId = FONT_ID_BODY_16;
                pieConfig.explodeDistance = 8.0f;
                
                // Apply current color mode
                pieConfig.colorMode = pieChartColorMode;
                switch (pieChartColorMode) {
                    case CLAY_PIECHART_COLOR_MODE_GRADIENT:
                        pieConfig.colorConfig.gradient = (Clay_PieChart_Gradient){
                            .start = COLOR_BLUE,
                            .end = COLOR_ORANGE
                        };
                        break;
                    case CLAY_PIECHART_COLOR_MODE_PALETTE:
                        pieConfig.colorConfig.palette = (Clay_PieChart_Palette){
                            .colors = pieChartPalette,
                            .count = len(pieChartPalette)
                        };
                        break;
                    case CLAY_PIECHART_COLOR_MODE_RANDOM:
                        pieConfig.colorConfig.random = (Clay_PieChart_Random){ .seed = 12345 };
                        break;
                    default:
                        break;
                }
                
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_GROW(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
                    }
                }) {
                    Clay_PieChart_Render(CLAY_STRING("SalesPieChart"), &pieConfig, 600, 450);
                }
            }
        }
        
        // Footer with instructions
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { .width = CLAY_SIZING_GROW(0) },
                .padding = { 16, 16, 16, 16 }
            },
            .backgroundColor = {255, 255, 255, 255},
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {
            CLAY_TEXT(
                CLAY_STRING("Clay Extension System Demo - Press 1-4 to change pie chart colors: (1) Per-segment (2) Palette (3) Gradient (4) Random"),
                CLAY_TEXT_CONFIG({
                    .fontSize = 14,
                    .fontId = FONT_ID_BODY_16,
                    .textColor = {100, 100, 110, 255}
                })
            );
        }
    }
    
    return Clay_EndLayout();
}

// Error handling
void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

int main(void) {
    // Initialize random seed
    srand(time(NULL));
    
    // Initialize Clay memory
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(
        clayMemory, 
        (Clay_Dimensions) { 1400, 900 }, 
        (Clay_ErrorHandler) { HandleClayErrors, 0 }
    );
    
    // Initialize Raylib
    Clay_Raylib_Initialize(
        1400, 
        900, 
        "Clay Extensions - Bar Chart Demo", 
        FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT
    );
    
    // Load fonts
    Font fonts[1];
    fonts[FONT_ID_BODY_16] = LoadFontEx("resources/Roboto-Regular.ttf", 48, 0, 400);
    SetTextureFilter(fonts[FONT_ID_BODY_16].texture, TEXTURE_FILTER_BILINEAR);
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);
    
    // Initialize sales data
    InitializeSalesData();
    
    // Main render loop
    while (!WindowShouldClose()) {
        // Update sales data every second
        UpdateSalesData();
        
        // Handle keyboard input for color mode switching
        if (IsKeyPressed(KEY_ONE)) {
            pieChartColorMode = CLAY_PIECHART_COLOR_MODE_PER_SEGMENT;
        } else if (IsKeyPressed(KEY_TWO)) {
            pieChartColorMode = CLAY_PIECHART_COLOR_MODE_PALETTE;
        } else if (IsKeyPressed(KEY_THREE)) {
            pieChartColorMode = CLAY_PIECHART_COLOR_MODE_GRADIENT;
        } else if (IsKeyPressed(KEY_FOUR)) {
            pieChartColorMode = CLAY_PIECHART_COLOR_MODE_RANDOM;
        } else if (IsKeyPressed(KEY_D)) {
            Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
        }
        
        // Update Clay dimensions for window resize
        Clay_SetLayoutDimensions((Clay_Dimensions) { 
            (float)GetScreenWidth(), 
            (float)GetScreenHeight() 
        });
        
        // Generate layout
        Clay_RenderCommandArray renderCommands = CreateLayout();
        
        // Render
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Render Clay layout first
        Clay_Raylib_Render(renderCommands, fonts);
        
        // Draw pie chart on top (after Clay renders)
        Clay_PieChart_Draw(CLAY_STRING("SalesPieChart"), &pieConfig, renderCommands);
        
        EndDrawing();
    }
    
    // Cleanup
    Clay_Raylib_Close();
    return 0;
}
