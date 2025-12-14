#ifndef EXTENSIONS_H
#define EXTENSIONS_H

/**
 * Clay Extensions Loader
 * 
 * USAGE:
 * Before including this file, define which extensions you want using the helper macros:
 * 
 *    #define CLAY_USE_BARCHART
 *    #define CLAY_USE_PIECHART
 *    #include "extensions/extensions.h"
 * 
 * This will automatically define the IMPLEMENTATION macros and include the headers.
 */

#ifdef CLAY_USE_BARCHART
    #define CLAY_EXTEND_BARCHART_IMPLEMENTATION
    #include "clay_extensions_barchart.h"
#endif

#ifdef CLAY_USE_PIECHART
    #define CLAY_EXTEND_PIECHART_IMPLEMENTATION
    #include "clay_extensions_piechart.h"
#endif

// Add more extensions here as needed:
// #ifdef CLAY_USE_LINECHART
//     #define CLAY_EXTENSIONS_LINECHART_IMPLEMENTATION
//     #include "clay_extensions_linechart.h"
// #endif

#endif // EXTENSIONS_H