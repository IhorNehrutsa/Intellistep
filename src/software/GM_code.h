#ifndef _GM_code_H__
#define _GM_code_H__

#include "config.h"

// Enumeration for G code distance mode
typedef enum {
    ABSOLUTE = 90,
    INCREMENTAL = 91
} DISTANCE_MODE;

typedef enum {
    X_AXIS = 'X', // main linear axes
    Y_AXIS = 'Y',
    Z_AXIS = 'Z',

    A_AXIS = 'A', // rotary axes
    B_AXIS = 'B',
    C_AXIS = 'C',

    U_AXIS = 'U', // additional axes
    V_AXIS = 'V',
    W_AXIS = 'W'
} AXES;

// GM code class stores a variables
class GM_code {

    public:

        // Initialize
        GM_code();

        // Keeps the current G code distance_mode
        DISTANCE_MODE distance_mode = ABSOLUTE;

        // Keeps the current G code rate for G6, G0 in Hz
        int32_t rate = DEFAULT_STEPPING_RATE;

    private:

};

#endif
