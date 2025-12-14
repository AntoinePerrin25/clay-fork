/**
 * Clay Extension: Pie Chart
 * 
 * A header-only extension for rendering pie charts using Clay layout.
 * This extension provides easy-to-use components for proportional data visualization.
 * 
 * Usage:
 *   #define CLAY_EXTEND_PIECHART_IMPLEMENTATION
 *   #include "clay_extend_piechart.h"
 * 
 * Features:
 *   - Pie and donut charts
 *   - Customizable colors and sizing
 *   - Automatic percentage calculation
 *   - Optional labels and legends
 *   - Exploded segments
 */

#ifndef CLAY_EXTEND_PIECHART_H
#define CLAY_EXTEND_PIECHART_H

#include "../../../clay.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef CLAY_EXTEND_PIECHART_IMPLEMENTATION
#include "raylib.h"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// PUBLIC API - Configuration Structures
// ============================================================================

typedef struct {
    float value;
    Clay_String label;
    Clay_Color color;
    bool exploded;  // Whether this segment is separated from center
} Clay_PieChart_DataPoint;

typedef enum {
    CLAY_PIECHART_COLOR_MODE_PER_SEGMENT,   // Each segment uses its own color from DataPoint
    CLAY_PIECHART_COLOR_MODE_PALETTE,       // Cycle through a palette of colors
    CLAY_PIECHART_COLOR_MODE_GRADIENT,      // Gradient between two colors
    CLAY_PIECHART_COLOR_MODE_RANDOM         // Random color for each segment
} Clay_PieChart_ColorMode;

typedef struct {
    Clay_Color *colors;
    uint32_t count;
} Clay_PieChart_Palette;

typedef struct {
    Clay_Color start;
    Clay_Color end;
} Clay_PieChart_Gradient;

typedef struct {
    uint32_t seed;  // 0 = use time-based seed
} Clay_PieChart_Random;

typedef struct {
    Clay_PieChart_DataPoint *data;
    uint32_t dataCount;
    float radius;            // Radius of the pie chart
    float donutHoleRadius;   // Inner radius (0 = pie chart, >0 = donut chart)
    float explodeDistance;   // Distance to offset exploded segments
    bool showLabels;         // Show segment labels
    bool showValues;         // Show actual values on segments
    bool showPercentages;    // Show percentages on segments
    bool showLegend;         // Show legend with colors and labels
    bool showSectorLines;    // Draw lines between sectors
    Clay_Color backgroundColor;
    Clay_Color labelTextColor;
    Clay_Color sectorLineColor; // Color for lines between sectors
    uint16_t labelFontSize;
    uint16_t labelFontId;
    float startAngle;        // Starting angle in degrees (0 = right, 90 = top)
    // Color mode configuration
    Clay_PieChart_ColorMode colorMode;
    union {
        Clay_PieChart_Palette palette;
        Clay_PieChart_Gradient gradient;
        Clay_PieChart_Random random;
    } colorConfig;
} Clay_PieChart_Config;

// ============================================================================
// PUBLIC API - Functions
// ============================================================================

/**
 * Render a pie chart component
 * 
 * @param id Unique ID for this chart element
 * @param config Configuration for the pie chart
 * @param width Width of the chart container
 * @param height Height of the chart container
 */
void Clay_PieChart_Render(Clay_String id, Clay_PieChart_Config *config, float width, float height);

/**
 * Draw the actual pie chart graphics (call this after Clay layout but before Clay_Raylib_Render)
 * 
 * @param chartId The element ID of the main chart container (same as used in Clay_PieChart_Render)
 * @param config Configuration for the pie chart
 * @param renderCommands The render command array to find element bounds
 */
void Clay_PieChart_Draw(Clay_String chartId, Clay_PieChart_Config *config, Clay_RenderCommandArray renderCommands);

/**
 * Create a default pie chart configuration
 */
Clay_PieChart_Config Clay_PieChart_DefaultConfig(void);

// ============================================================================
// IMPLEMENTATION
// ============================================================================

#ifdef CLAY_EXTEND_PIECHART_IMPLEMENTATION

// Internal helper: Calculate total value from data
static float Clay_PieChart__CalculateTotalValue(Clay_PieChart_Config *config) {
    float total = 0.0f;
    for (uint32_t i = 0; i < config->dataCount; i++) {
        total += config->data[i].value;
    }
    return total;
}

// Internal helper: Convert degrees to radians
static float Clay_PieChart__DegreesToRadians(float degrees) {
    return degrees * M_PI / 180.0f;
}

// Internal helper: Lerp between two colors
static Clay_Color Clay_PieChart__LerpColor(Clay_Color start, Clay_Color end, float t) {
    return (Clay_Color) {
        .r = start.r + (end.r - start.r) * t,
        .g = start.g + (end.g - start.g) * t,
        .b = start.b + (end.b - start.b) * t,
        .a = start.a + (end.a - start.a) * t
    };
}

// Internal helper: Simple pseudo-random number generator
static uint32_t Clay_PieChart__Random(uint32_t *seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return *seed;
}

// Internal helper: Get color for a segment based on color mode
static Clay_Color Clay_PieChart__GetSegmentColor(Clay_PieChart_Config *config, uint32_t index) {
    switch (config->colorMode) {
        case CLAY_PIECHART_COLOR_MODE_PER_SEGMENT:
            return config->data[index].color;
            
        case CLAY_PIECHART_COLOR_MODE_PALETTE:
            if (config->colorConfig.palette.colors && config->colorConfig.palette.count > 0) {
                return config->colorConfig.palette.colors[index % config->colorConfig.palette.count];
            }
            return config->data[index].color;
            
        case CLAY_PIECHART_COLOR_MODE_GRADIENT: {
            float t = config->dataCount > 1 ? (float)index / (float)(config->dataCount - 1) : 0.0f;
            return Clay_PieChart__LerpColor(config->colorConfig.gradient.start, config->colorConfig.gradient.end, t);
        }
            
        case CLAY_PIECHART_COLOR_MODE_RANDOM: {
            static uint32_t seed = 0;
            if (seed == 0) {
                seed = config->colorConfig.random.seed != 0 ? config->colorConfig.random.seed : (uint32_t)time(NULL);
            }
            uint32_t rnd = Clay_PieChart__Random(&seed);
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

// Internal helper: Render a pie segment using Raylib
static void Clay_PieChart__RenderSegment(
    Clay_PieChart_DataPoint *dataPoint,
    uint32_t index,
    float centerX,
    float centerY,
    float startAngleDeg,
    float sweepAngleDeg,
    Clay_PieChart_Config *config,
    bool isLast
) {
    // Calculate offset for exploded segments
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    if (dataPoint->exploded) {
        float midAngleRad = Clay_PieChart__DegreesToRadians(startAngleDeg + sweepAngleDeg / 2.0f);
        offsetX = cosf(midAngleRad) * config->explodeDistance;
        offsetY = sinf(midAngleRad) * config->explodeDistance;
    }
    
    // Convert Clay_Color to Raylib Color
    Clay_Color segmentClayColor = Clay_PieChart__GetSegmentColor(config, index);
    Color segmentColor = (Color){ 
        segmentClayColor.r, 
        segmentClayColor.g, 
        segmentClayColor.b, 
        segmentClayColor.a 
    };
    
    // Draw the pie segment using Raylib
    Vector2 center = { centerX + offsetX, centerY + offsetY };
    
    if (config->donutHoleRadius > 0.0f) {
        // Draw donut segment (ring)
        DrawRing(center, config->donutHoleRadius, config->radius, startAngleDeg, startAngleDeg + sweepAngleDeg, 32, segmentColor);
    } else {
        // Draw pie segment
        DrawCircleSector(center, config->radius, startAngleDeg, startAngleDeg + sweepAngleDeg, 32, segmentColor);
    }
    
    // Draw sector line
    if (config->showSectorLines && !isLast) {
        float endAngleDeg = startAngleDeg + sweepAngleDeg;
        float endAngleRad = Clay_PieChart__DegreesToRadians(endAngleDeg);
        
        Color lineColor = (Color){
            config->sectorLineColor.r,
            config->sectorLineColor.g,
            config->sectorLineColor.b,
            config->sectorLineColor.a
        };
        
        float innerRadius = config->donutHoleRadius;
        float outerRadius = config->radius;
        
        Vector2 innerPoint = {
            center.x + cosf(endAngleRad) * innerRadius,
            center.y + sinf(endAngleRad) * innerRadius
        };
        Vector2 outerPoint = {
            center.x + cosf(endAngleRad) * outerRadius,
            center.y + sinf(endAngleRad) * outerRadius
        };
        
        DrawLineEx(innerPoint, outerPoint, 2.0f, lineColor);
    }
}

// Internal helper: Render legend
static void Clay_PieChart__RenderLegend(Clay_PieChart_Config *config) {
    CLAY(
        CLAY_ID("PieLegend"),
        {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_FIT(0),
                    .height = CLAY_SIZING_FIT(0)
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 8
            },
            .backgroundColor = config->backgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }
    ) {
        float totalValue = Clay_PieChart__CalculateTotalValue(config);
        
        for (uint32_t i = 0; i < config->dataCount; i++) {
            CLAY(
                CLAY_IDI("LegendItem", i),
                {
                    .layout = {
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .sizing = {
                            .width = CLAY_SIZING_FIT(0),
                            .height = CLAY_SIZING_FIT(0)
                        },
                        .childGap = 8,
                        .childAlignment = {
                            .y = CLAY_ALIGN_Y_CENTER
                        }
                    }
                }
            ) {
                // Color box
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(16),
                            .height = CLAY_SIZING_FIXED(16)
                        }
                    },
                    .backgroundColor = Clay_PieChart__GetSegmentColor(config, i),
                    .cornerRadius = CLAY_CORNER_RADIUS(2)
                }) {}
                
                // Label
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_FIT(0) }
                    }
                }) {
                    CLAY_TEXT(
                        config->data[i].label,
                        CLAY_TEXT_CONFIG({
                            .fontSize = config->labelFontSize,
                            .fontId = config->labelFontId,
                            .textColor = config->labelTextColor
                        })
                    );
                }
                
                // Percentage
                static char percentBuffers[32][32];  // Array for each item
                float percentage = (config->data[i].value / totalValue) * 100.0f;
                int len = snprintf(percentBuffers[i], sizeof(percentBuffers[i]), "(%.1f%%)", percentage);
                Clay_String percentString = (Clay_String) {
                    .isStaticallyAllocated = false,
                    .length = len,
                    .chars = percentBuffers[i]
                };
                
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_FIT(0) }
                    }
                }) {
                    CLAY_TEXT(
                        percentString,
                        CLAY_TEXT_CONFIG({
                            .fontSize = config->labelFontSize - 2,
                            .fontId = config->labelFontId,
                            .textColor = (Clay_Color){120, 120, 120, 255}
                        })
                    );
                }
            }
        }
    }
}

// Main render function
void Clay_PieChart_Render(Clay_String id, Clay_PieChart_Config *config, float width, float height) {
    if (config->dataCount == 0 || config->data == NULL) {
        return;
    }
    
    float totalValue = Clay_PieChart__CalculateTotalValue(config);
    if (totalValue <= 0.0f) {
        return;
    }
    
    // Main chart container
    CLAY(
        CLAY_SID(id),
        {
            .layout = {
                .layoutDirection = config->showLegend ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_FIXED(width),
                    .height = CLAY_SIZING_FIXED(height)
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 24,
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                }
            },
            .backgroundColor = config->backgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }
    ) {
        // Pie chart area - use a custom element to mark where to draw
        CLAY(
            CLAY_ID("PieArea"),
            {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(config->radius * 2 + config->explodeDistance * 2),
                        .height = CLAY_SIZING_FIXED(config->radius * 2 + config->explodeDistance * 2)
                    }
                },
                .backgroundColor = (Clay_Color){0, 0, 0, 0}  // Transparent
            }
        ) {}
        
        // Legend (if enabled)
        if (config->showLegend) {
            Clay_PieChart__RenderLegend(config);
        }
    }
}

// Draw the actual pie chart (call after Clay_EndLayout)
void Clay_PieChart_Draw(Clay_String chartId, Clay_PieChart_Config *config, Clay_RenderCommandArray renderCommands) {
    if (config->dataCount == 0 || config->data == NULL) {
        fprintf(stderr, "PieChart: No data (count=%d, data=%p)\n", config->dataCount, config->data);
        return;
    }
    
    float totalValue = Clay_PieChart__CalculateTotalValue(config);
    if (totalValue <= 0.0f) {
        return;
    }
    
    // Find the main chart container element in render commands
    Clay_BoundingBox chartBounds = {0};
    bool found = false;
    Clay_ElementId chartElementId = Clay_GetElementId(chartId);
    
    for (uint32_t i = 0; i < renderCommands.length; i++) {
        Clay_RenderCommand *cmd = &renderCommands.internalArray[i];
        if (cmd->id == chartElementId.id) {
            chartBounds = cmd->boundingBox;
            found = true;
            break;
        }
    }
    
    if (!found) {
        fprintf(stderr, "PieChart: Element not found! Tried ID %u\n", chartElementId.id);
        fprintf(stderr, "PieChart: First 5 element IDs in render commands:\n");
        for (uint32_t i = 0; i < 5 && i < renderCommands.length; i++) {
            fprintf(stderr, "  [%d] ID: %u\n", i, renderCommands.internalArray[i].id);
        }
        return;
    }
    
    // Calculate center of pie chart area
    // Account for padding and legend if present
    float pieAreaSize = config->radius * 2 + config->explodeDistance * 2;
    float centerX, centerY;
    
    if (config->showLegend) {
        // Legend is on the right, pie is on the left side
        centerX = chartBounds.x + 16 + pieAreaSize / 2.0f;  // padding + half size
        centerY = chartBounds.y + chartBounds.height / 2.0f;
    } else {
        // Centered
        centerX = chartBounds.x + chartBounds.width / 2.0f;
        centerY = chartBounds.y + chartBounds.height / 2.0f;
    }
    
    float currentAngleDeg = config->startAngle;
    
    for (uint32_t i = 0; i < config->dataCount; i++) {
        float sweepAngleDeg = (config->data[i].value / totalValue) * 360.0f;
        
        Clay_PieChart__RenderSegment(
            &config->data[i],
            i,
            centerX,
            centerY,
            currentAngleDeg,
            sweepAngleDeg,
            config,
            i == config->dataCount - 1  // isLast
        );
        
        currentAngleDeg += sweepAngleDeg;
    }
}

// Default configuration helper
Clay_PieChart_Config Clay_PieChart_DefaultConfig(void) {
    return (Clay_PieChart_Config) {
        .data = NULL,
        .dataCount = 0,
        .radius = 120.0f,
        .donutHoleRadius = 0.0f,
        .explodeDistance = 10.0f,
        .showLabels = false,
        .showValues = false,
        .showPercentages = true,
        .showLegend = true,
        .showSectorLines = true,
        .backgroundColor = (Clay_Color){ 255, 255, 255, 255 },
        .labelTextColor = (Clay_Color){ 60, 60, 60, 255 },
        .sectorLineColor = (Clay_Color){ 255, 255, 255, 200 },
        .labelFontSize = 14,
        .labelFontId = 0,
        .startAngle = -90.0f,  // Start at top (12 o'clock)
        .colorMode = CLAY_PIECHART_COLOR_MODE_PER_SEGMENT,
        .colorConfig = {
            .gradient = {
                .start = (Clay_Color){ 100, 150, 250, 255 },
                .end = (Clay_Color){ 250, 100, 150, 255 }
            }
        }
    };
}

#endif // CLAY_EXTEND_PIECHART_IMPLEMENTATION

#endif // CLAY_EXTEND_PIECHART_H
