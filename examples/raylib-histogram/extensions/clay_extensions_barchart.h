/**
 * Clay Extension: Bar Chart / Histogram
 * 
 * A header-only extension for rendering bar charts and histograms using Clay layout.
 * This extension provides easy-to-use components for data visualization.
 * 
 * Usage:
 *   #define CLAY_EXTEND_BARCHART_IMPLEMENTATION
 *   #include "clay_extend_barchart.h"
 * 
 * Features:
 *   - Vertical and horizontal bar charts
 *   - Customizable colors, spacing, and sizing
 *   - Automatic scaling and labeling
 *   - Responsive layout using Clay's sizing system
 */

#ifndef CLAY_EXTEND_BARCHART_H
#define CLAY_EXTEND_BARCHART_H

#include "../../../clay.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// PUBLIC API - Configuration Structures
// ============================================================================

typedef CLAY_PACKED_ENUM {
    CLAY_BARCHART_ORIENTATION_VERTICAL,
    CLAY_BARCHART_ORIENTATION_HORIZONTAL
} Clay_BarChart_Orientation;

// Forward declaration for config wrapper
typedef struct Clay_BarChart_Config Clay_BarChart_Config;

typedef struct {
    float value;
    Clay_String label;
    Clay_Color color;
} Clay_BarChart_DataPoint;

typedef CLAY_PACKED_ENUM {
    CLAY_BARCHART_COLOR_MODE_PER_BAR,      // Each bar uses its own color from DataPoint
    CLAY_BARCHART_COLOR_MODE_PALETTE,      // Cycle through a palette of colors
    CLAY_BARCHART_COLOR_MODE_GRADIENT,     // Gradient between two colors
    CLAY_BARCHART_COLOR_MODE_RANDOM        // Random color for each bar
} Clay_BarChart_ColorMode;

typedef struct {
    Clay_Color *colors;
    uint32_t count;
} Clay_BarChart_Palette;

typedef struct {
    Clay_Color start;
    Clay_Color end;
} Clay_BarChart_Gradient;

typedef struct {
    uint32_t seed;  // 0 = use time-based seed
} Clay_BarChart_Random;

struct Clay_BarChart_Config {
    Clay_BarChart_DataPoint *data;
    uint32_t dataCount;
    Clay_BarChart_Orientation orientation;
    float barWidth;          // Width of each bar (for vertical charts) or height (for horizontal)
    float barGap;            // Spacing between bars
    float maxValue;          // Max value for scaling (0 = auto-calculate)
    Clay_Color backgroundColor;
    Clay_Color gridColor;
    Clay_Color labelTextColor;
    uint16_t labelFontSize;
    uint16_t labelFontId;
    bool showGrid;
    bool showLabels;
    bool showValues;
    // Color mode configuration
    Clay_BarChart_ColorMode colorMode;
    union {
        Clay_BarChart_Palette palette;
        Clay_BarChart_Gradient gradient;
        Clay_BarChart_Random random;
    } colorConfig;
};

// Wrapper struct for macro-based configuration (similar to Clay's pattern)
CLAY__WRAPPER_STRUCT(Clay_BarChart_Config);

// ============================================================================
// PUBLIC API - Macros (Clay-style API)
// ============================================================================

/**
 * CLAY_BARCHART_CONFIG - Create a bar chart configuration inline
 * Usage: CLAY_BARCHART_CONFIG({ .data = myData, .dataCount = 12, ... })
 */
#define CLAY_BARCHART_CONFIG(...) Clay__StoreBarChartConfig(CLAY__CONFIG_WRAPPER(Clay_BarChart_Config, __VA_ARGS__))

/**
 * CLAY_BARCHART - Render a bar chart element (Clay-style macro)
 * Usage: 
 *   CLAY_BARCHART(CLAY_ID("MyChart"), CLAY_BARCHART_CONFIG({
 *       .data = salesData,
 *       .dataCount = 12,
 *       .orientation = CLAY_BARCHART_ORIENTATION_VERTICAL,
 *       .showLabels = true
 *   }));
 */
#define CLAY_BARCHART(id, config) Clay_BarChart__RenderWithId(id, config)

// ============================================================================
// PUBLIC API - Functions
// ============================================================================

/**
 * Render a bar chart component
 * Auto-sizes to fill parent container with padding.
 * 
 * @param id Unique ID for this chart element
 * @param config Configuration for the bar chart
 */
void Clay_BarChart_Render(Clay_String id, Clay_BarChart_Config *config);

/**
 * Render a bar chart with Clay_ElementId (for macro API)
 */
void Clay_BarChart__RenderWithId(Clay_ElementId id, Clay_BarChart_Config *config);

/**
 * Store bar chart config (for macro API, similar to Clay__StoreTextElementConfig)
 */
Clay_BarChart_Config* Clay__StoreBarChartConfig(Clay_BarChart_Config config);

/**
 * Create a default bar chart configuration
 */
Clay_BarChart_Config Clay_BarChart_DefaultConfig(void);

// ============================================================================
// IMPLEMENTATION
// ============================================================================

#ifdef CLAY_EXTEND_BARCHART_IMPLEMENTATION

#include <time.h>

// Storage for bar chart configs (circular buffer, resets each frame)
static Clay_BarChart_Config Clay__BarChartConfigBuffer[32];
static uint32_t Clay__BarChartConfigIndex = 0;

// Reset config buffer index (call at start of each frame/layout)
static void Clay_BarChart__ResetConfigBuffer(void) {
    Clay__BarChartConfigIndex = 0;
}

// Store bar chart config and return pointer
Clay_BarChart_Config* Clay__StoreBarChartConfig(Clay_BarChart_Config config) {
    // Wrap around if we exceed buffer size
    if (Clay__BarChartConfigIndex >= 32) {
        Clay__BarChartConfigIndex = 0;
    }
    
    // Apply defaults for unset values
    if (config.labelFontSize == 0) config.labelFontSize = 16;
    if (config.backgroundColor.a == 0 && config.backgroundColor.r == 0 && 
        config.backgroundColor.g == 0 && config.backgroundColor.b == 0) {
        config.backgroundColor = (Clay_Color){ 245, 245, 245, 255 };
    }
    if (config.labelTextColor.a == 0 && config.labelTextColor.r == 0 &&
        config.labelTextColor.g == 0 && config.labelTextColor.b == 0) {
        config.labelTextColor = (Clay_Color){ 60, 60, 60, 255 };
    }
    if (config.barWidth == 0) config.barWidth = 60.0f;
    if (config.barGap == 0) config.barGap = 8.0f;
    
    Clay__BarChartConfigBuffer[Clay__BarChartConfigIndex] = config;
    return &Clay__BarChartConfigBuffer[Clay__BarChartConfigIndex++];
}

// Internal helper: Calculate maximum value from data
static float Clay_BarChart__CalculateMaxValue(Clay_BarChart_Config *config) {
    if (config->maxValue > 0.0f) {
        return config->maxValue;
    }
    
    float max = 0.0f;
    for (uint32_t i = 0; i < config->dataCount; i++) {
        if (config->data[i].value > max) {
            max = config->data[i].value;
        }
    }
    
    // Add 10% padding to max value for better visualization
    return max * 1.1f;
}

// Internal helper: Lerp between two colors
static Clay_Color Clay_BarChart__LerpColor(Clay_Color start, Clay_Color end, float t) {
    return (Clay_Color) {
        .r = start.r + (end.r - start.r) * t,
        .g = start.g + (end.g - start.g) * t,
        .b = start.b + (end.b - start.b) * t,
        .a = start.a + (end.a - start.a) * t
    };
}

// Internal helper: Simple pseudo-random number generator
static uint32_t Clay_BarChart__Random(uint32_t *seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return *seed;
}

// Internal helper: Get color for a bar based on color mode
static Clay_Color Clay_BarChart__GetBarColor(Clay_BarChart_Config *config, uint32_t index) {
    switch (config->colorMode) {
        case CLAY_BARCHART_COLOR_MODE_PER_BAR:
            return config->data[index].color;
            
        case CLAY_BARCHART_COLOR_MODE_PALETTE:
            if (config->colorConfig.palette.colors && config->colorConfig.palette.count > 0) {
                return config->colorConfig.palette.colors[index % config->colorConfig.palette.count];
            }
            return config->data[index].color;
            
        case CLAY_BARCHART_COLOR_MODE_GRADIENT: {
            float t = config->dataCount > 1 ? (float)index / (float)(config->dataCount - 1) : 0.0f;
            return Clay_BarChart__LerpColor(config->colorConfig.gradient.start, config->colorConfig.gradient.end, t);
        }
            
        case CLAY_BARCHART_COLOR_MODE_RANDOM: {
            static uint32_t seed = 0;
            if (seed == 0) {
                seed = config->colorConfig.random.seed != 0 ? config->colorConfig.random.seed : (uint32_t)time(NULL);
            }
            uint32_t rnd = Clay_BarChart__Random(&seed);
            return (Clay_Color) {
                .r = 100 + (rnd % 156),
                .g = 100 + ((rnd >> 8) % 156),
                .b = 100 + ((rnd >> 16) % 156),
                .a = 255
            };
        }
            
        default:
            return config->data[index].color;
    }
}

// Internal helper: Render a single bar (vertical orientation)
static void Clay_BarChart__RenderVerticalBar(
    Clay_BarChart_DataPoint *dataPoint,
    uint32_t index,
    float calculatedBarHeight,
    float labelHeight,
    Clay_BarChart_Config *config
) {
    // Static buffer array to store value strings for each bar
    static char valueBuffers[32][32];
    
    // Container for bar and label - uses calculated fixed height
    CLAY(
        CLAY_IDI("BarV", index),
        {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(4.0f), // Bar width: 4 units for 4:1 ratio with gap
                    .height = CLAY_SIZING_FIXED(calculatedBarHeight + labelHeight)
                },
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_BOTTOM
                },
                .childGap = 4
            }
        }
    ) {
        // Bar area with value on top - grows to fill minus label space
        CLAY_AUTO_ID({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_FIXED(calculatedBarHeight)
                },
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_BOTTOM
                },
                .childGap = 4
            }
        }) {
            // Value label (if enabled)
            if (config->showValues) {
                snprintf(valueBuffers[index], sizeof(valueBuffers[index]), "%.1f", dataPoint->value);
                Clay_String valueString = (Clay_String) {
                    .isStaticallyAllocated = false,
                    .length = strlen(valueBuffers[index]),
                    .chars = valueBuffers[index]
                };
                CLAY_TEXT(
                    valueString,
                    CLAY_TEXT_CONFIG({
                        .fontSize = config->labelFontSize,
                        .fontId = config->labelFontId,
                        .textColor = config->labelTextColor
                    })
                );
            }
            
            // The actual bar - grows to fill remaining space
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0)
                    }
                },
                .backgroundColor = Clay_BarChart__GetBarColor(config, index),
                .cornerRadius = CLAY_CORNER_RADIUS(4)
            }) {}
        }
        
        // Label below bar (if enabled) - fixed height
        if (config->showLabels && dataPoint->label.length > 0) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = { .height = CLAY_SIZING_FIXED(labelHeight) }
                }
            }) {
                CLAY_TEXT(
                    dataPoint->label,
                    CLAY_TEXT_CONFIG({
                        .fontSize = config->labelFontSize,
                        .fontId = config->labelFontId,
                        .textColor = config->labelTextColor
                    })
                );
            }
        }
    }
}

// Internal helper: Render a single bar (horizontal orientation)
static void Clay_BarChart__RenderHorizontalBar(
    Clay_BarChart_DataPoint *dataPoint,
    uint32_t index,
    float barHeight,
    float maxWidth,
    Clay_BarChart_Config *config
) {
    // Static buffer array to store value strings for each bar
    static char valueBuffers[32][32];
    
    // Calculate bar width based on value
    float scaledWidth = (dataPoint->value / Clay_BarChart__CalculateMaxValue(config)) * maxWidth;
    
    // Container for bar and label
    CLAY(
        CLAY_IDI("BarH", index),
        {
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_FIXED(barHeight)
                },
                .childAlignment = {
                    .x = CLAY_ALIGN_X_LEFT,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 8
            }
        }
    ) {
        // Label on the left (if enabled)
        if (config->showLabels && dataPoint->label.length > 0) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = { .width = CLAY_SIZING_FIXED(80) }
                }
            }) {
                CLAY_TEXT(
                    dataPoint->label,
                    CLAY_TEXT_CONFIG({
                        .fontSize = config->labelFontSize,
                        .fontId = config->labelFontId,
                        .textColor = config->labelTextColor
                    })
                );
            }
        }
        
        // The actual bar
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(scaledWidth),
                    .height = CLAY_SIZING_FIXED(barHeight - 8)
                }
            },
            .backgroundColor = Clay_BarChart__GetBarColor(config, index),
            .cornerRadius = CLAY_CORNER_RADIUS(4)
        }) {}
        
        // Value label on the right (if enabled)
        if (config->showValues) {
            snprintf(valueBuffers[index], sizeof(valueBuffers[index]), "%.1f", dataPoint->value);
            Clay_String valueString = (Clay_String) {
                .isStaticallyAllocated = false,
                .length = strlen(valueBuffers[index]),
                .chars = valueBuffers[index]
            };
            CLAY_TEXT(
                valueString,
                CLAY_TEXT_CONFIG({
                    .fontSize = config->labelFontSize,
                    .fontId = config->labelFontId,
                    .textColor = config->labelTextColor
                })
            );
        }
    }
}

// Main render function - auto-sizes to parent container
void Clay_BarChart_Render(Clay_String id, Clay_BarChart_Config *config) {
    if (config->dataCount == 0 || config->data == NULL) {
        return;
    }
    
    // Calculate max value and label heights
    float maxValue = Clay_BarChart__CalculateMaxValue(config);
    float labelHeight = config->showLabels ? config->labelFontSize + 4 : 0;
    
    // Reference maximum bar height (tallest bar will be this height)
    // This represents available height minus padding and labels
    float referenceMaxHeight = 350.0f; // Adjust based on typical container size
    
    // Main chart container - grows to fill parent with padding
    CLAY(
        CLAY_SID(id),
        {
            .layout = {
                .layoutDirection = config->orientation == CLAY_BARCHART_ORIENTATION_VERTICAL 
                    ? CLAY_LEFT_TO_RIGHT 
                    : CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0)
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 0, // No gap - we add manual spacers with GROW(1)
                .childAlignment = config->orientation == CLAY_BARCHART_ORIENTATION_VERTICAL
                    ? (Clay_ChildAlignment){ .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_BOTTOM }
                    : (Clay_ChildAlignment){ .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = config->backgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }
    ) {
        if (config->orientation == CLAY_BARCHART_ORIENTATION_VERTICAL) {
            // Render vertical bars with calculated heights
            for (uint32_t i = 0; i < config->dataCount; i++) {
                // Calculate bar height as proportion of max height
                float barHeightRatio = config->data[i].value / maxValue;
                float calculatedBarHeight = barHeightRatio * referenceMaxHeight;
                
                Clay_BarChart__RenderVerticalBar(
                    &config->data[i],
                    i,
                    calculatedBarHeight,
                    labelHeight,
                    config
                );
                
                // Add gap spacer between bars (not after last bar)
                if (i < config->dataCount - 1) {
                    CLAY(
                        CLAY_IDI("BarGap", i),
                        {
                            .layout = {
                                .sizing = {
                                    .width = CLAY_SIZING_GROW(1.0f) // Gap: 1 unit for 4:1 ratio
                                }
                            }
                        }
                    ) {}
                }
            }
        } else {
            // Render horizontal bars
            for (uint32_t i = 0; i < config->dataCount; i++) {
                Clay_BarChart__RenderHorizontalBar(
                    &config->data[i],
                    i,
                    config->barWidth,
                    config->data[i].value,
                    config
                );
            }
        }
    }
}

// Internal render function using Clay_ElementId (for macro API)
static void Clay_BarChart__RenderInternal(Clay_ElementId elementId, Clay_BarChart_Config *config) {
    if (config->dataCount == 0 || config->data == NULL) {
        return;
    }
    
    // Calculate max value and label heights
    float maxValue = Clay_BarChart__CalculateMaxValue(config);
    float labelHeight = config->showLabels ? config->labelFontSize + 4 : 0;
    
    // Reference maximum bar height (tallest bar will be this height)
    float referenceMaxHeight = 350.0f;
    
    // Main chart container - grows to fill parent with padding
    CLAY(
        elementId,
        {
            .layout = {
                .layoutDirection = config->orientation == CLAY_BARCHART_ORIENTATION_VERTICAL 
                    ? CLAY_LEFT_TO_RIGHT 
                    : CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0)
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 0,
                .childAlignment = config->orientation == CLAY_BARCHART_ORIENTATION_VERTICAL
                    ? (Clay_ChildAlignment){ .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_BOTTOM }
                    : (Clay_ChildAlignment){ .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = config->backgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }
    ) {
        if (config->orientation == CLAY_BARCHART_ORIENTATION_VERTICAL) {
            for (uint32_t i = 0; i < config->dataCount; i++) {
                float barHeightRatio = config->data[i].value / maxValue;
                float calculatedBarHeight = barHeightRatio * referenceMaxHeight;
                
                Clay_BarChart__RenderVerticalBar(
                    &config->data[i],
                    i,
                    calculatedBarHeight,
                    labelHeight,
                    config
                );
                
                if (i < config->dataCount - 1) {
                    CLAY(
                        CLAY_IDI("BarGap", i),
                        {
                            .layout = {
                                .sizing = {
                                    .width = CLAY_SIZING_GROW(1.0f)
                                }
                            }
                        }
                    ) {}
                }
            }
        } else {
            for (uint32_t i = 0; i < config->dataCount; i++) {
                Clay_BarChart__RenderHorizontalBar(
                    &config->data[i],
                    i,
                    config->barWidth,
                    config->data[i].value,
                    config
                );
            }
        }
    }
}

// Render function for macro API (takes Clay_ElementId)
void Clay_BarChart__RenderWithId(Clay_ElementId id, Clay_BarChart_Config *config) {
    Clay_BarChart__RenderInternal(id, config);
}

// Default configuration helper
Clay_BarChart_Config Clay_BarChart_DefaultConfig(void) {
    return (Clay_BarChart_Config) {
        .data = NULL,
        .dataCount = 0,
        .orientation = CLAY_BARCHART_ORIENTATION_VERTICAL,
        .barWidth = 60.0f,
        .barGap = 8.0f,
        .maxValue = 0.0f, // Auto-calculate
        .backgroundColor = (Clay_Color){ 245, 245, 245, 255 },
        .gridColor = (Clay_Color){ 200, 200, 200, 255 },
        .labelTextColor = (Clay_Color){ 60, 60, 60, 255 },
        .labelFontSize = 16,
        .labelFontId = 0,
        .showGrid = false,
        .showLabels = true,
        .showValues = true,
        .colorMode = CLAY_BARCHART_COLOR_MODE_PER_BAR,
        .colorConfig = {
            .gradient = {
                .start = (Clay_Color){ 100, 150, 250, 255 },
                .end = (Clay_Color){ 250, 100, 150, 255 }
            }
        }
    };
}

#endif // CLAY_EXTEND_BARCHART_IMPLEMENTATION

#endif // CLAY_EXTEND_BARCHART_H
