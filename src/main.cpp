#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/GameLevelManager.hpp>

using namespace geode::prelude;

class $modify(MyLevelBrowserLayer, LevelBrowserLayer) {
    struct Fields {
        cocos2d::CCArray* aggregated = nullptr; 
        bool alreadyFiltered = false; 
        int targetCount = 10;
    int maxFetches = 6; 
    std::string lastSig;
    int nextToFetch = -1;
    std::unordered_set<int> seenIDs; 
    };

    bool asIsEasySelected(GJSearchObject* so) {
        if (!so) return false;
        std::string diffTokens = "," + std::string(so->m_difficulty.c_str()) + ",";
        return diffTokens.find(",1,") != std::string::npos;
    }

    GJSearchObject* asCloneWithPage(GJSearchObject* so, int page) {
        if (!so) return nullptr;
        auto key = so->getKey();
        if (!key) return nullptr;
        auto copy = GJSearchObject::createFromKey(key);
        if (!copy) return nullptr;
        copy->m_page = page;
        return copy;
    }
    void asResetAggregation() {
        if (m_fields->aggregated) {
            m_fields->aggregated->release();
            m_fields->aggregated = nullptr;
        }
        m_fields->alreadyFiltered = false;
    m_fields->maxFetches = 6;
    m_fields->nextToFetch = -1;
    m_fields->seenIDs.clear();
    }
    void asAppendNonAuto(cocos2d::CCArray* src) {
        if (!src) return;
        if (!m_fields->aggregated) {
            m_fields->aggregated = cocos2d::CCArray::create();
            m_fields->aggregated->retain();
        }
        for (int i = 0; i < src->count(); i++) {
            auto level = static_cast<GJGameLevel*>(src->objectAtIndex(i));
            if (!level) continue;
            if (level->m_autoLevel) continue;
            int id = level->m_levelID;
            if (m_fields->seenIDs.insert(id).second) {
                m_fields->aggregated->addObject(level); 
            }
        }
    }

    void asFinalizePageSlice(const char* tag, int type) {
        int total = m_fields->aggregated ? m_fields->aggregated->count() : 0;
        int page = this->m_searchObject ? this->m_searchObject->m_page : 0;
        int start = page * m_fields->targetCount; 
        if (start >= total && total > 0) {
            start = std::max(0, total - m_fields->targetCount);
        }
        auto finalArr = cocos2d::CCArray::create();
        for (int i = start; i < total && finalArr->count() < m_fields->targetCount; i++) {
            finalArr->addObject(m_fields->aggregated->objectAtIndex(i));
        }
        m_fields->alreadyFiltered = true; 
        LevelBrowserLayer::loadLevelsFinished(finalArr, tag, type);
    }

    void setupLevelBrowser(cocos2d::CCArray* levels) {
        
        if (!levels) {
            LevelBrowserLayer::setupLevelBrowser(levels);
            return;
        }
        
        if (!this->m_searchObject) {
            LevelBrowserLayer::setupLevelBrowser(levels);
            return;
        }
        
        auto searchObject = this->m_searchObject;

    if (m_fields->alreadyFiltered) {
            LevelBrowserLayer::setupLevelBrowser(levels);
            return;
        }

    std::string diffTokens = "," + std::string(searchObject->m_difficulty.c_str()) + ",";
    bool easySelected = diffTokens.find(",1,") != std::string::npos; 


        if (easySelected) {
            for (int i = levels->count() - 1; i >= 0; i--) {
                auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(i));
                if (!level) continue;
                if (level->m_autoLevel) {
                    levels->removeObjectAtIndex(i);
                }
            }
            LevelBrowserLayer::setupLevelBrowser(levels);
            return;
        }
        LevelBrowserLayer::setupLevelBrowser(levels);
    }
    
    void loadPage(GJSearchObject* searchObject) {
        auto makeSig = [](GJSearchObject* so) -> std::string {
            if (!so) return std::string();
            std::string s;
            s.reserve(256);
            auto addb = [&](bool b){ s += b ? '1' : '0'; s += '|'; };
            s += std::to_string(static_cast<int>(so->m_searchType)); s += '|';
            s += so->m_searchQuery.c_str(); s += '|';
            s += so->m_difficulty.c_str(); s += '|';
            s += so->m_length.c_str(); s += '|';
            addb(so->m_starFilter);
            addb(so->m_uncompletedFilter);
            addb(so->m_featuredFilter);
            s += std::to_string(so->m_songID); s += '|';
            addb(so->m_originalFilter);
            addb(so->m_twoPlayerFilter);
            addb(so->m_customSongFilter);
            addb(so->m_songFilter);
            addb(so->m_noStarFilter);
            addb(so->m_coinsFilter);
            addb(so->m_epicFilter);
            addb(so->m_legendaryFilter);
            addb(so->m_mythicFilter);
            s += std::to_string(static_cast<int>(so->m_demonFilter)); s += '|';
            s += std::to_string(so->m_folder); s += '|';
            s += std::to_string(so->m_searchMode);
            return s;
        };

    auto sig = makeSig(searchObject);
    if (!m_fields->lastSig.size() || sig != m_fields->lastSig || (searchObject && searchObject->m_page == 0)) {
            asResetAggregation();
            m_fields->lastSig = sig;
        }
    m_fields->alreadyFiltered = false;
        if (searchObject) {
            int desiredNext = searchObject->m_page + 1;
            if (m_fields->nextToFetch < 0) m_fields->nextToFetch = desiredNext;
            else m_fields->nextToFetch = std::max(m_fields->nextToFetch, desiredNext);
        }
        LevelBrowserLayer::loadPage(searchObject);
    }

    void loadLevelsFinished(cocos2d::CCArray* arr, char const* tag, int type) {
        if (!this->m_searchObject || !asIsEasySelected(this->m_searchObject)) {
            asResetAggregation();
            LevelBrowserLayer::loadLevelsFinished(arr, tag, type);
            return;
        }
        if (!m_fields->aggregated) {
            m_fields->aggregated = cocos2d::CCArray::create();
            m_fields->aggregated->retain();
        }
        asAppendNonAuto(arr);
        int page = this->m_searchObject ? this->m_searchObject->m_page : 0;
        int needUpTo = (page + 1) * m_fields->targetCount;
        int have = m_fields->aggregated->count();

        asFinalizePageSlice(tag, type);

        if (have < needUpTo && m_fields->maxFetches > 0) {
            int nextPage = (m_fields->nextToFetch >= 0) ? m_fields->nextToFetch : (page + 1);
            auto next = asCloneWithPage(this->m_searchObject, nextPage);
            if (next) {
                m_fields->maxFetches--;
                m_fields->nextToFetch = nextPage + 1;
                GameLevelManager::sharedState()->getOnlineLevels(next);
            }
        }
    }
};