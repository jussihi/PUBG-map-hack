#pragma once
#include "KReader.hpp"
#include "CURLWrapper.hpp"
#include "Types.hpp"
#include <deque>
#include <nmmintrin.h>

class GameDataParser
{
public:
	GameDataParser()
	{
		m_kReader = new KReader;
		m_CURL = new CURLWrapper;
	}
	~GameDataParser()
	{
		delete m_kReader;
		delete m_CURL;
	}


	void readLoop()
	{
		json data;

		data["players"] = json::array();
		data["vehicles"] = json::array();
		data["items"] = json::array();

		readLocals();
		readPlayers(data);

		if (m_CURL->getReadyState() && !(data["items"].empty() && data["vehicles"].empty() && data["players"].empty()))
		{
			m_CURL->sendData(data.dump());
		}

	}

	int64_t getPUBase()
	{
		return m_kReader->getPUBase();
	}

	int64_t readPUBase()
	{
		return m_kReader->readPUBase();
	}
	

private:

	/*
	 * PRIVATE CLASS FUNCTIONS
	 */
	struct TSLEncryptDataObj
	{
		uint64_t ptrs[0x2B];
		uint16_t enc_index;
		byte  unk2[0x6];
		uint16_t enc_xor;
		byte  unk3[0x6];
	};
	
	void readPlayers(json& w_data)
	{
		for (int i = 0; i < m_actorCount; i++)
		{
			// read the position of Player
			auto encrypted_actor = m_kReader->readType<TSLEncryptDataObj>(m_AActorPtr + (i * sizeof(TSLEncryptDataObj)), PROTO_NORMAL_READ);
    			uint16_t decoded_xor = encrypted_actor.enc_xor ^ 0xCBAC;
			uint16_t decoded_index = _mm_crc32_u32(0x19D5D75Fu, encrypted_actor.enc_index ^ 0x3185721C) % 0x2B;
			int64_t curActor = decoded_xor ^ encrypted_actor.ptrs[decoded_index];
			//int64_t curActor = m_kReader->readType<int64_t>(m_AActorPtr + (i * 0x8), PROTO_NORMAL_READ);
			int32_t curActorID = m_kReader->readType<int32_t>(curActor + 0x0018, PROTO_NORMAL_READ);
			std::string actorGName = m_kReader->getGNameFromId(curActorID);

			// Here we check if the name is found from the wanted GNames list (PlayerMale etc...)
			if (std::find(playerIDs.begin(), playerIDs.end(), curActorID) != playerIDs.end())
			{
				int64_t rootCmpPtr = m_kReader->readType<int64_t>(curActor + 0x180, PROTO_NORMAL_READ);
				int64_t playerState = m_kReader->readType<int64_t>(curActor + 0x3C0, PROTO_NORMAL_READ);
				Vector3 actorLocation = m_kReader->readVec(rootCmpPtr + 0x1A0, PROTO_NORMAL_READ);

				int32_t actorTeam = m_kReader->readType<int32_t>(playerState + 0x0444, PROTO_NORMAL_READ);

				actorLocation.X += m_kReader->readType<int32_t>(m_PWorld + 0x918, PROTO_NORMAL_READ);
				actorLocation.Y += m_kReader->readType<int32_t>(m_PWorld + 0x91C, PROTO_NORMAL_READ);
				// actorLocation.Z += ReadAny<int>(PWorld + 0x920, PROTO_NORMAL_READ);

				w_data["players"].emplace_back(json::object({ { "t", actorTeam },{ "x", actorLocation.X },{ "y", actorLocation.Y }/*,{ "z", actorLocation.Z }*/ }));
			}

			if (actorGName == "DroppedItemGroup" || actorGName == "DroppedItemInteractionComponent")
			{
				int64_t rootCmpPtr = m_kReader->readType<int64_t>(curActor + 0x180, PROTO_NORMAL_READ);
				int64_t playerState = m_kReader->readType<int64_t>(curActor + 0x3C0, PROTO_NORMAL_READ);
				Vector3 actorLocation = m_kReader->readVec(rootCmpPtr + 0x1A0, PROTO_NORMAL_READ);
				int64_t DroppedItemArray = m_kReader->readType<int64_t>(curActor + 0x2D8, PROTO_NORMAL_READ);
				int32_t DroppedItemCount = m_kReader->readType<int32_t>(curActor + 0x2E0, PROTO_NORMAL_READ);

				for (int j = 0; j < DroppedItemCount; j++)
				{
					int64_t ADroppedItem = m_kReader->readType<int64_t>(DroppedItemArray + j * 0x10, PROTO_NORMAL_READ);
					Vector3 droppedLocation = m_kReader->readVec(ADroppedItem + 0x1E0, PROTO_NORMAL_READ);
					droppedLocation.X = droppedLocation.X + actorLocation.X + m_kReader->readType<int32_t>(m_PWorld + 0x918, PROTO_NORMAL_READ);
					droppedLocation.Y = droppedLocation.Y + actorLocation.Y + m_kReader->readType<int32_t>(m_PWorld + 0x91C, PROTO_NORMAL_READ);
					int64_t UItem = m_kReader->readType<int64_t>(ADroppedItem + 0x448, PROTO_NORMAL_READ);
					int32_t UItemID = m_kReader->readType<int32_t>(UItem + 0x18, PROTO_NORMAL_READ);
					std::string itemName = m_kReader->getGNameFromId(UItemID);

					// check if inside the map / array of wanted items
					for (std::map<std::string, std::string>::iterator it = dropGNameMap.begin(); it != dropGNameMap.end(); ++it)
					{
						if (itemName.substr(0, it->first.length()) == it->first)
						{
							int64_t rootCmpPtr = m_kReader->readType<int64_t>(curActor + 0x180, PROTO_NORMAL_READ);
							Vector3 actorLocation = m_kReader->readVec(rootCmpPtr + 0x1A0, PROTO_NORMAL_READ);

							actorLocation.X += m_kReader->readType<int32_t>(m_PWorld + 0x918, PROTO_NORMAL_READ);
							actorLocation.Y += m_kReader->readType<int32_t>(m_PWorld + 0x91C, PROTO_NORMAL_READ);

							w_data["items"].emplace_back(json::object({ { "n", it->second },{ "x", droppedLocation.X },{ "y", droppedLocation.Y } }));
						}
					}
				}
			}

			else if (actorGName.substr(0, strlen("CarePackage")) == "CarePackage" || actorGName.substr(0, strlen("AircraftCarePackage")) == "AircraftCarePackage" || actorGName.substr(0, strlen("Carapackage_RedBox")) == "Carapackage_RedBox")
			{
				int64_t rootCmpPtr = m_kReader->readType<int64_t>(curActor + 0x180, PROTO_NORMAL_READ);
				int64_t playerState = m_kReader->readType<int64_t>(curActor + 0x3C0, PROTO_NORMAL_READ);
				Vector3 actorLocation = m_kReader->readVec(rootCmpPtr + 0x1A0, PROTO_NORMAL_READ);

				actorLocation.X += m_kReader->readType<int32_t>(m_PWorld + 0x918, PROTO_NORMAL_READ);
				actorLocation.Y += m_kReader->readType<int32_t>(m_PWorld + 0x91C, PROTO_NORMAL_READ);

				w_data["vehicles"].emplace_back(json::object({ { "v", "Drop" },{ "x", actorLocation.X },{ "y", actorLocation.Y } }));

			}

			else if (std::find(allIDs.begin(), allIDs.end(), curActorID) == allIDs.end())
			{
				allIDs.push_back(curActorID);

				if (actorGName == "FAIL")
				{
					continue;
				}
				else
				{
					// iterate thru playerGnameVec
					for (std::vector<std::string>::iterator it = playerGNameVec.begin(); it != playerGNameVec.end(); ++it)
					{
						//check if the name is same, and add it to the playerIDs vector
						if (*it == actorGName.substr(0, (*it).length()))
						{
							playerIDs.push_back(curActorID);
							break;
						}
					}

					// iterate thru vehicleGNameVec
					for (std::vector<std::string>::iterator it = vehicleGNameVec.begin(); it != vehicleGNameVec.end(); ++it)
					{
						//check if the name is same, and add it to the vehicleIDs vector
						if (*it == actorGName.substr(0, (*it).length()))
						{
							vehicleIDs.push_back(curActorID);
							break;
						}
					}
				}
			}
			else if (std::find(vehicleIDs.begin(), vehicleIDs.end(), curActorID) != vehicleIDs.end())
			{
				// tästä alaspäin voi tehdä if-lohkoissa
				int64_t rootCmpPtr = m_kReader->readType<int64_t>(curActor + 0x180, PROTO_NORMAL_READ);
				Vector3 actorLocation = m_kReader->readVec(rootCmpPtr + 0x1A0, PROTO_NORMAL_READ);

				actorLocation.X += m_kReader->readType<int32_t>(m_PWorld + 0x918, PROTO_NORMAL_READ);
				actorLocation.Y += m_kReader->readType<int32_t>(m_PWorld + 0x91C, PROTO_NORMAL_READ);

				std::string carName = m_kReader->getGNameFromId(curActorID);

				w_data["vehicles"].emplace_back(json::object({ { "v", carName.substr(0, 3) },{ "x", actorLocation.X },{ "y", actorLocation.Y } }));

			}
		}
	}

	void readLocals()
	{
		m_UWorld = m_kReader->readType<int64_t>(m_kReader->getPUBase() + 0x3A5D218, PROTO_NORMAL_READ);
		m_gameInstance = m_kReader->readType<int64_t>(m_UWorld + 0x140, PROTO_NORMAL_READ);
		m_ULocalPlayer = m_kReader->readType<int64_t>(m_gameInstance + 0x38, PROTO_NORMAL_READ);
		m_localPlayer = m_kReader->readType<int64_t>(m_ULocalPlayer, PROTO_NORMAL_READ);
		m_viewportclient = m_kReader->readType<int64_t>(m_localPlayer + 0x58, PROTO_NORMAL_READ);
		m_localPawn = m_kReader->readType<int64_t>(m_localPlayer + 0x3A8, PROTO_NORMAL_READ);
		m_localPlayerState = m_kReader->readType<int64_t>(m_localPawn + 0x3C0, PROTO_NORMAL_READ);
		m_PWorld = m_kReader->readType<int64_t>(m_viewportclient + 0x80, PROTO_NORMAL_READ);
		m_ULevel = m_kReader->readType<int64_t>(m_PWorld + 0x30, PROTO_NORMAL_READ);
		m_actorCount = m_kReader->readType<int32_t>(m_ULevel + 0xA8, PROTO_NORMAL_READ);

		m_localPlayerPosition = m_kReader->readVec(m_localPlayer + 0x70, PROTO_NORMAL_READ);
		m_localPlayerBasePointer = m_kReader->readType<int64_t>(m_localPlayer, PROTO_NORMAL_READ);

		m_localTeam = m_kReader->readType<int32_t>(m_localPlayerState + 0x0444, PROTO_NORMAL_READ);

		m_AActorPtr = m_kReader->readType<int64_t>(m_ULevel + 0xA0, PROTO_NORMAL_READ);
	}



	/*
	 * CLASS VARIABLES
	 */
	KReader* m_kReader;
	CURLWrapper* m_CURL;


	/*
	 * Local variables
	 * These are updated once every read loop.
	 */
	int64_t m_UWorld;
	int64_t m_gameInstance;
	int64_t m_ULocalPlayer;
	int64_t m_localPlayer;
	int64_t m_viewportclient;
	int64_t m_localPawn;
	int64_t m_localPlayerState;
	int64_t m_PWorld;
	int64_t m_ULevel;
	int32_t m_actorCount;
	Vector3 m_localPlayerPosition;
	int64_t m_localPlayerBasePointer;
	int32_t m_localTeam;
	int64_t m_AActorPtr;

	/*
	 * Global IDs that are found from the game
	 * These containers are used to help the 
	 * maintaining of systematic ID handling and 
	 * storing.
	 */
	std::deque<int32_t> allIDs;

	std::vector<int32_t> playerIDs;
	std::vector<int32_t> vehicleIDs;
};
