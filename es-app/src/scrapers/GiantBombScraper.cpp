#include "scrapers/GiantBombScraper.h"
#include "Log.h"
#include "pugixml/pugixml.hpp"
#include "MetaData.h"
#include "Settings.h"
#include "Util.h"
#include <boost/assign.hpp>

using namespace PlatformIds;
const std::map<PlatformId, const int> gamesdb_platformid_map = boost::assign::map_list_of
	(THREEDO, 26)
	(AMIGA, 1)
	(AMSTRAD_CPC, 11)
	{APPLE2, 12}
	(ARCADE, 84)
	//(ATARI_800, 0)
	(ATARI_2600, 40)
	(ATARI_5200, 67)
	(ATARI_7800, 70)
	(ATARI_JAGUAR, 28)
	//(ATARI_JAGUAR_CD, 0)
	(ATARI_LYNX, 7)
	(ATARI_ST, 13)
	//(ATARI_STE, 0)
	//(ATARI_FALCON, 0)
	//(ATARI_XE, "Atari XE")
	(COLECOVISION, 47)
	(COMMODORE_64, 14)
	(INTELLIVISION, 51)
	(MAC_OS, 17)
	(XBOX, 32)
	(XBOX_360, 20)
	(MSX, 15)
	(NEOGEO, 25)
	(NEOGEO_POCKET, 80)
	(NEOGEO_POCKET_COLOR, 81)
	(NINTENDO_3DS, 117)
	(NINTENDO_64, 43)
	(NINTENDO_DS, 52)
	(NINTENDO_ENTERTAINMENT_SYSTEM, 21)
	(GAME_BOY, 3)
	(GAME_BOY_ADVANCE, 4)
	(GAME_BOY_COLOR, 57)
	(NINTENDO_GAMECUBE, 23)
	(NINTENDO_WII, 36)
	(NINTENDO_WII_U, 139)
	(PC, 94)
	(SEGA_32X, 31)
	(SEGA_CD, 29)
	(SEGA_DREAMCAST, 37)
	(SEGA_GAME_GEAR, 5)
	(SEGA_GENESIS, 6)
	(SEGA_MASTER_SYSTEM, 8)
	(SEGA_MEGA_DRIVE, 6)
	(SEGA_SATURN, 42)
	(PLAYSTATION, 22)
	(PLAYSTATION_2, 19)
	(PLAYSTATION_3, 35)
	(PLAYSTATION_4, 146)
	(PLAYSTATION_VITA, 129)
	(PLAYSTATION_PORTABLE, 18)
	(SUPER_NINTENDO, 9)
	(TURBOGRAFX_16, 55)
	(WONDERSWAN, 65)
	(WONDERSWAN_COLOR, 54)
	(ZX_SPECTRUM, 16);
	
void giantbomb_generate_scraper_requests(const ScraperSearchParams& params, std::queue< std::unique_ptr<ScraperRequest> >& requests, 
	std::vector<ScraperSearchResult>& results)
{
	const std::string& apiKey = Settings::getInstance()->getString("ScraperAPIKey");
	
	if(apiKey.empty()) {
		std::string err =  "GiantBomb scraper warning - no API key set";
		setError(err);
		LOG(LogError) << err;
		return;
	}

	std::string path = "giantbomb.com/api/game?api_key=";
	path += apiKey;
	path += "&format=xml&resources=game&filter=name:%";

	std::string cleanName = params.nameOverride;
	if(cleanName.empty()) {
		cleanName = params.game->getCleanName();
	}

	path += HttpReq::urlEncode(cleanName);
	path += "%";
	
	if(params.system->getPlatformIds().empty())	{
		requests.push(std::unique_ptr<ScraperRequest>(new TheGamesDBRequest(results, path)));
	} else {
		std::string urlBase = path;
		auto& platforms = params.system->getPlatformIds();
		for(auto platformIt = platforms.begin(); platformIt != platforms.end(); platformIt++)
		{
			path = urlBase;
			auto mapIt = gamesdb_platformid_map.find(*platformIt);
			if(mapIt != gamesdb_platformid_map.end())
			{
				path += ",platform=" + boost::lexical_cast<std::string>(mapIt->second);
			}else{
				LOG(LogWarning) << "TheGamesDB scraper warning - no support for platform " << getPlatformName(*platformIt);
			}

			requests.push(std::unique_ptr<ScraperRequest>(new TheGamesDBRequest(results, path)));
		}
	}
}

void GiantBombRequest::process(const std::unique_ptr<HttpReq>& req, std::vector<ScraperSearchResult>& results)
{
	assert(req->status() == HttpReq::REQ_SUCCESS);

	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load(req->getContent().c_str());
	if(!parseResult)
	{
		std::stringstream ss;
		ss << "GiantBombRequest - Error parsing XML. \n\t" << parseResult.description() << "";
		std::string err = ss.str();
		setError(err);
		LOG(LogError) << err;
		return;
	}

	pugi::xml_node data = doc.child("response").child("results");

	pugi::xml_node game = data.child("game");
	while(game && results.size() < MAX_SCRAPER_RESULTS)
	{
		ScraperSearchResult result;

		result.mdl.set("name", game.child("name").text().get());
		result.mdl.set("desc", game.child("deck").text().get());
		pugi::xml_node images = game.child("image");

		if(images)
		{
			// icon_url, medium_url, screen_url, small_url, super_url, thumb_url, tiny_url
			result.thumbnailUrl = images.child("thumb_url").text().get());
			result.imageUrl =  images.child("screen_url").text().get());
		}

		results.push_back(result);
		game = game.next_sibling("Game");
	}
}
