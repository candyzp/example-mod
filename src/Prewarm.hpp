#pragma once

class PlayLayer;

namespace cbfplus {
    void beginLevelSession(PlayLayer* layer);
    void prewarmLevel(PlayLayer* layer);
    void endLevelSession(PlayLayer* layer);
}
