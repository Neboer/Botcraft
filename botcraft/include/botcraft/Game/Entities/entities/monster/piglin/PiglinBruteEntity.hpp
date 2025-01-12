#if PROTOCOL_VERSION > 736
#pragma once

#include "botcraft/Game/Entities/entities/monster/piglin/AbstractPiglinEntity.hpp"

namespace Botcraft
{
    class PiglinBruteEntity : public AbstractPiglinEntity
    {
    protected:
        static constexpr int metadata_count = 0;
        static constexpr int hierarchy_metadata_count = AbstractPiglinEntity::metadata_count + AbstractPiglinEntity::hierarchy_metadata_count;

    public:
        PiglinBruteEntity();
        virtual ~PiglinBruteEntity();

        // Object related stuff
        virtual std::string GetName() const override;
        virtual EntityType GetType() const override;
        virtual double GetWidth() const override;
        virtual double GetHeight() const override;

        // Static stuff, for easier comparison
        static std::string GetClassName();
        static EntityType GetClassType();

    };
}
#endif
