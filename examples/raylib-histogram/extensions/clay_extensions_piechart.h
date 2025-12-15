/**
 * Clay Extension: Pie Chart
 * 
 * A header-only extension for rendering pie charts using Clay's custom element system.
 * This extension provides easy-to-use components for proportional data visualization.
 * 
 * Usage:
 *   #define CLAY_EXTEND_PIECHART_IMPLEMENTATION
 *   #include "clay_extensions_piechart.h"
 * 
 * Features:
 *   - Pie and donut charts as native Clay custom elements
 *   - Customizable colors and sizing
 *   - Automatic percentage calculation
 *   - Optional labels and legends
 *   - Exploded segments
 *   - Integrated with Clay's layout and rendering system
 * 
 * Example:
 *   Clay_PieChart_DataPoint data[] = {
 *       { .value = 30, .label = CLAY_STRING("Red"), .color = COLOR_RED },
 *       { .value = 50, .label = CLAY_STRING("Blue"), .color = COLOR_BLUE },
 *       { .value = 20, .label = CLAY_STRING("Green"), .color = COLOR_GREEN }
 *   };
 *   
 *   Clay_PieChart_Config config = Clay_PieChart_DefaultConfig();
 *   config.data = data;
 *   config.dataCount = 3;
 *   
 *   CLAY_PIECHART(CLAY_STRING("MyChart"), &config, 400, 400);
 *   
 *   // Then in your render loop, handle CLAY_RENDER_COMMAND_TYPE_CUSTOM:
 *   case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
 *       Clay_PieChart_RenderCustomElement(cmd);
 *       break;
 *   }
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

// Custom element type identifier for piechart
#define CLAY_PIECHART_CUSTOM_ELEMENT_TYPE 0x50494543 // 'PIEC' in ASCII

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

// Internal data structure passed through Clay's custom element system
typedef struct {
    uint32_t elementType;  // Always CLAY_PIECHART_CUSTOM_ELEMENT_TYPE
    Clay_PieChart_Config config;
    float totalValue;  // Pre-calculated total for efficiency
    uint32_t configHash;  // Hash of config for caching
    RenderTexture2D texture;  // Cached render texture
    bool textureValid;  // Whether texture needs regeneration
} Clay_PieChart_CustomElementData;

// ============================================================================
// PUBLIC API - Functions
// ============================================================================

/**
 * Render a pie chart as a Clay custom element
 * Auto-sizes to fill parent container with padding.
 * 
 * @param id Unique ID for this chart element
 * @param config Configuration for the pie chart (will be copied)
 * 
 * Note: The config data will be stored in Clay's arena and passed through
 * to the render commands. Make sure your data pointers remain valid.
 */
void Clay_PieChart_Render(Clay_String id, Clay_PieChart_Config *config);

/**
 * Macro for easier pie chart creation with automatic ID handling
 * 
 * @param id Clay_String ID for this chart
 * @param config Pointer to Clay_PieChart_Config
 * @param width Width in pixels
 * @param height Height in pixels
 * 
 * Example: CLAY_PIECHART(CLAY_STRING("Sales"), &config, 400, 400);
 */
#define CLAY_PIECHART(id, config, width, height) Clay_PieChart_Render(id, config, width, height)

/**
 * Render a pie chart custom element to texture (call before Clay_Raylib_Render)
 * This generates/updates the texture if data changed
 * 
 * @param renderCommand The render command from Clay's output
 */
void Clay_PieChart_PrepareTexture(Clay_RenderCommand *renderCommand);

/**
 * Get the texture for a pie chart custom element (used by renderer)
 * 
 * @param renderCommand The render command from Clay's output
 * @return Pointer to RenderTexture2D or NULL if not a pie chart
 */
RenderTexture2D* Clay_PieChart_GetTexture(Clay_RenderCommand *renderCommand);

/**
 * Create a default pie chart configuration
 */
Clay_PieChart_Config Clay_PieChart_DefaultConfig(void);

// ============================================================================
// IMPLEMENTATION
// ============================================================================

#ifdef CLAY_EXTEND_PIECHART_IMPLEMENTATION

// Internal helper: Simple hash function for config data
static uint32_t Clay_PieChart__HashConfig(Clay_PieChart_Config *config) {
    uint32_t hash = 2166136261u; // FNV-1a offset basis
    
    // Hash data values and basic config
    for (uint32_t i = 0; i < config->dataCount; i++) {
        // Hash value
        uint32_t valueInt = *(uint32_t*)&config->data[i].value;
        hash ^= valueInt;
        hash *= 16777619u;
        
        // Hash exploded flag
        hash ^= config->data[i].exploded ? 1 : 0;
        hash *= 16777619u;
    }
    
    // Hash config parameters
    hash ^= *(uint32_t*)&config->radius;
    hash *= 16777619u;
    hash ^= *(uint32_t*)&config->donutHoleRadius;
    hash *= 16777619u;
    hash ^= config->colorMode;
    hash *= 16777619u;
    hash ^= config->showSectorLines ? 1 : 0;
    hash *= 16777619u;
    
    return hash;
}

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

// Internal helper: Render legend as standard Clay elements
static void Clay_PieChart__RenderLegend(Clay_PieChart_Config *config, float totalValue) {
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
                static char percentBuffers[32][32];  // Static array for each legend item
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

// Main render function - creates a Clay custom element
// Main render function - creates a Clay custom element, auto-sizes to parent
void Clay_PieChart_Render(Clay_String id, Clay_PieChart_Config *config) {
    if (config->dataCount == 0 || config->data == NULL) {
        return;
    }
    
    float totalValue = Clay_PieChart__CalculateTotalValue(config);
    if (totalValue <= 0.0f) {
        return;
    }
    
    // Allocate custom element data in static storage
    // Note: This uses simple static storage. In production you might want to use
    // Clay's arena or your own allocation mechanism
    static Clay_PieChart_CustomElementData elementDataStorage;
    
    Clay_PieChart_CustomElementData *elementData = &elementDataStorage;

    // Compute a quick hash for the incoming config so we can detect changes.
    uint32_t newConfigHash = Clay_PieChart__HashConfig(config);
    bool configChanged = (elementData->configHash != newConfigHash);

    // If the config changed and we previously had a valid texture, unload it now.
    if (configChanged && elementData->textureValid && elementData->texture.id != 0) {
        UnloadRenderTexture(elementData->texture);
        elementData->textureValid = false;
    }

    // Store the config data (copy) and update metadata. Preserve existing
    // texture if the config didn't change so we avoid unnecessary re-rendering.
    elementData->elementType = CLAY_PIECHART_CUSTOM_ELEMENT_TYPE;
    elementData->config = *config;  // Copy the config
    elementData->totalValue = totalValue;
    elementData->configHash = newConfigHash;

    if (configChanged) {
        // Will be created/updated on first PrepareTexture after change
        elementData->textureValid = false;
        elementData->texture = (RenderTexture2D){0};  // Initialize to zero
    }
    
    // Main chart container - grows to fill parent
    CLAY(
        CLAY_SID(id),
        {
            .layout = {
                .layoutDirection = config->showLegend ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0)
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
        // Pie chart area - create a custom element
        CLAY(
            CLAY_ID("PieChartCustom"),
            {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(config->radius * 2 + config->explodeDistance * 2),
                        .height = CLAY_SIZING_FIXED(config->radius * 2 + config->explodeDistance * 2)
                    }
                },
                .custom = {
                    .customData = elementData
                }
            }
        ) {}
        
        // Legend (if enabled)
        if (config->showLegend) {
            Clay_PieChart__RenderLegend(config, totalValue);
        }
    }
}

// Prepare texture for rendering (call before Clay_Raylib_Render)
void Clay_PieChart_PrepareTexture(Clay_RenderCommand *renderCommand) {
    // Verify this is a pie chart custom element
    Clay_PieChart_CustomElementData *elementData = 
        (Clay_PieChart_CustomElementData *)renderCommand->renderData.custom.customData;
    
    if (!elementData || elementData->elementType != CLAY_PIECHART_CUSTOM_ELEMENT_TYPE) {
        return;  // Not a pie chart element, skip
    }
    
    Clay_PieChart_Config *config = &elementData->config;
    float totalValue = elementData->totalValue;
    
    if (config->dataCount == 0 || config->data == NULL || totalValue <= 0.0f) {
        return;
    }
    
    // Calculate required texture size from actual bounding box
    // Use 2x resolution for better quality (supersampling)
    Clay_BoundingBox bounds = renderCommand->boundingBox;
    int texWidth = (int)(bounds.width * 2.0f);
    int texHeight = (int)(bounds.height * 2.0f);
    
    // Ensure minimum size for quality
    if (texWidth < 128) texWidth = 128;
    if (texHeight < 128) texHeight = 128;
    
    // Check if we need to regenerate texture
    uint32_t currentHash = Clay_PieChart__HashConfig(config);
    bool needsRegeneration = !elementData->textureValid || 
                             elementData->configHash != currentHash ||
                             elementData->texture.texture.width != texWidth ||
                             elementData->texture.texture.height != texHeight;
    
    if (needsRegeneration) {
        // Unload old texture if it exists
        if (elementData->textureValid && elementData->texture.id != 0) {
            UnloadRenderTexture(elementData->texture);
        }
        
        // Create new render texture
        elementData->texture = LoadRenderTexture(texWidth, texHeight);
        
        // Render pie chart to texture
        BeginTextureMode(elementData->texture);
        ClearBackground((Color){0, 0, 0, 0});  // Transparent background
        
        // Calculate scale factor to use full texture resolution
        float maxDimension = (config->radius * 2.0f + config->explodeDistance * 2.0f);
        float scaleX = texWidth / maxDimension;
        float scaleY = texHeight / maxDimension;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        
        float centerX = texWidth / 2.0f;
        float centerY = texHeight / 2.0f;
        float currentAngleDeg = config->startAngle;
        
        // Temporarily scale the config for high-res rendering
        Clay_PieChart_Config scaledConfig = *config;
        scaledConfig.radius *= scale;
        scaledConfig.explodeDistance *= scale;
        scaledConfig.donutHoleRadius *= scale;
        
        for (uint32_t i = 0; i < scaledConfig.dataCount; i++) {
            float sweepAngleDeg = (scaledConfig.data[i].value / totalValue) * 360.0f;
            
            Clay_PieChart__RenderSegment(
                &scaledConfig.data[i],
                i,
                centerX,
                centerY,
                currentAngleDeg,
                sweepAngleDeg,
                &scaledConfig,
                i == scaledConfig.dataCount - 1  // isLast
            );
            
            currentAngleDeg += sweepAngleDeg;
        }
        
        EndTextureMode();
        
        // Mark texture as valid and update hash
        elementData->textureValid = true;
        elementData->configHash = currentHash;
    }
}

// Get texture for rendering
RenderTexture2D* Clay_PieChart_GetTexture(Clay_RenderCommand *renderCommand) {
    Clay_PieChart_CustomElementData *elementData = 
        (Clay_PieChart_CustomElementData *)renderCommand->renderData.custom.customData;
    
    if (!elementData || elementData->elementType != CLAY_PIECHART_CUSTOM_ELEMENT_TYPE) {
        return NULL;
    }
    
    if (!elementData->textureValid) {
        return NULL;
    }
    
    return &elementData->texture;
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
