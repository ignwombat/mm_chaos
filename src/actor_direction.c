#include "z64actor.h"
#include "z64math.h"
#include "wommyutils.h"

Vec2f ActorDirection(Actor* actor) {
    // TODO change it to use velocity instead of facing direction, as the player can just walk backwards
    Vec2f result;
    s16 angle = actor->world.rot.y;

    result.x = Math_SinS(angle);
    result.z = Math_CosS(angle);

    return result;
}