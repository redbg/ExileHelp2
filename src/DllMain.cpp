#define _CRT_SECURE_NO_WARNINGS
#include "mongoose.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPoint>
#include <QtDebug>
#include <QtGlobal>
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <string>

DWORD64 PathOfExile = (DWORD64)GetModuleHandle(NULL);

// ====================================================================================================

struct WorldAreaInfo
{
    DWORD64 WorldArea;
    DWORD64 WorldAreas;

    WorldAreaInfo(DWORD hash16)
    {
        // "Data/WorldAreas.dat"
        auto GetWorldAreas = (void (*)(_Out_ DWORD64 * worldAreas))(PathOfExile + 0xD9FA0); // 3.18.0b

        auto GetWorldArea = (void (*)(_Out_ WorldAreaInfo * worldAreaInfo,
                                      DWORD * hash16,
                                      DWORD64 * worldAreas))(PathOfExile + 0x15EC560); // 3.18.0b

        GetWorldAreas(&this->WorldAreas);
        GetWorldArea(this, &hash16, &this->WorldAreas);
    }

    QString getId()
    {
        return QString::fromUtf16(*(char16_t **)WorldArea);
    }

    QString getName()
    {
        return QString::fromUtf16(*(char16_t **)(WorldArea + 8));
    }
};

// ====================================================================================================

template <typename T>
struct ExileVector
{
    T *Begin;
    T *End;
    T *Max;
};

struct Tile
{
    DWORD64 u1;
    DWORD64 u2;
    DWORD64 u3;
    DWORD64 u4;
    DWORD64 u5;
    DWORD64 u6;
    DWORD64 u7;

    QString getName()
    {
        return QString::fromUtf16(*(char16_t **)(u2 + 8));
    }

    int x()
    {
        return ((char *)(&u7))[4];
    }

    int y()
    {
        return ((char *)(&u7))[5];
    }

    QString getFullName()
    {
        return QString("%1x:%2-y:%3").arg(this->getName()).arg(x()).arg(y());
    }
};

struct Terrain
{
    union
    {
        struct
        {
            char              pad_18[0x18];
            DWORD64           Width;
            DWORD64           Height;
            ExileVector<Tile> Tiles;
        };

        struct
        {
            char              pad_d8[0xd8];
            ExileVector<char> MeleeLayer;
            ExileVector<char> RangedLayer;
            DWORD             BytesPerRow;
        };
    };

    void show()
    {
        for (auto tile = Tiles.Begin; tile != Tiles.End; tile++)
        {
            qDebug() << tile->getFullName();
        }
    }

    QPoint find(QString key)
    {
        for (auto tile = Tiles.Begin; tile != Tiles.End; tile++)
        {
            if (tile->getFullName() == key)
            {
                int i = (DWORD)(tile - Tiles.Begin);

                return QPoint(i % Width, i / Width);
            }
        }

        return QPoint();
    }
};

// ====================================================================================================

struct World
{
    union
    {
        struct
        {
            char          pad_80[0x80];
            WorldAreaInfo WorldInfo;
        };

        struct
        {
            char  pad_104[0x104];
            DWORD TileHash;
            DWORD DoodadHash;
        };

        struct
        {
            char    pad_650[0x650];
            DWORD64 GameObjectRegister;
        };

        struct
        {
            char    pad_940[0x940];
            Terrain terrain;
        };

        char pad[0x1000];
    };

    World(DWORD hash16, DWORD seed)
    {
        // 48 89 5C 24 10 48 89 4C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ?? 48 8B F9 E8
        auto InitWorld = (void (*)(_Out_ World * world))(PathOfExile + 0xC07DC0); // 3.18.0b

        // call r10
        auto GeneratingWorld = (void (*)(World * world, WorldAreaInfo * worldAreaInfo, DWORD seed,
                                         DWORD64 a4, DWORD64 a5, DWORD64 a6, DWORD64 a7,
                                         DWORD64 a8, DWORD64 a9, DWORD64 a10))(PathOfExile + 0xC05B40); // 3.18.0b

        WorldAreaInfo info(hash16);

        memset(this, 0, sizeof(World));

        InitWorld(this);

        char temp[0x1000] = {};
        GeneratingWorld(this, &info, seed,
                        (DWORD64)&temp[0],
                        (DWORD64)&temp[0x200],
                        (DWORD64)&temp[0x400],
                        (DWORD64)&temp[0x600],
                        (DWORD64)&temp[0x800],
                        (DWORD64)&temp[0xA00],
                        (DWORD64)&temp[0xC00]);
    }
    ~World() {}

    QString getRadarInfo()
    {
        static QJsonObject RadarData = getRadarData();

        QJsonObject keys = RadarData.value(this->WorldInfo.getId()).toObject();
        QJsonObject result;

        for (auto i = keys.begin(); i != keys.end(); i++)
        {
            QPoint      point = this->terrain.find(i.key());
            QJsonObject temp;
            temp.insert("x", point.x() * 23);
            temp.insert("y", point.y() * 23);
            result.insert(i.value().toString(), temp);
        }

        return QJsonDocument(result).toJson(QJsonDocument::Compact);
    }

    QJsonObject getRadarData()
    {
        QFile file("important_tgt_files.txt");
        file.open(QFile::ReadOnly);

        return QJsonDocument::fromJson(file.readAll()).object();
    }

    std::string getTerrain()
    {
        // 此处用 std::string 比 QByteArray 性能好
        std::string TerrainData;

        // 提前申请内存,优化性能
        TerrainData.reserve((terrain.Height * 23) * (terrain.BytesPerRow * 2) + 1);

        for (int y = 0; y < terrain.Height * 23; ++y)
        {
            for (int x = 0; x < terrain.BytesPerRow * 2; ++x)
            {
                char data = terrain.MeleeLayer.Begin[(y * terrain.BytesPerRow) + (x / 2)];
                data      = (x % 2) == 0 ? data & 0xf : data >> 4;
                TerrainData.append(1, data + '0');
            }
            TerrainData.append("\n");
        }

        return TerrainData;
    }
};

// ====================================================================================================

struct Component
{
    virtual void  vFun_0();
    virtual void  vFun_8();
    virtual void  vFun_10();
    virtual void  vFun_18();
    virtual void  vFun_20();
    virtual void  vFun_28();
    virtual void  vFun_30();
    virtual void  vFun_38();
    virtual void  vFun_40();
    virtual void  vFun_48();
    virtual void  vFun_50();
    virtual void  vFun_58();
    virtual char *getName();
    virtual void  vFun_68();
    virtual void  vFun_70();
    virtual void  vFun_78();
    virtual void  vFun_80();
    virtual void  vFun_88();
};

struct GameObject
{
    DWORD64                  vTable;
    DWORD64                  ObjectTypeInfo;
    ExileVector<Component *> Components;
    char                     buff[0x300];

    GameObject(DWORD hash)
    {
        // "Unknown object type serialized by server"
        auto GetGameObjectTypeArray = (DWORD64(*)(DWORD64 * GameObjectRegister))(PathOfExile + 0xB2410);                                                       // 3.18.0b
        auto GetGameObjectType      = (DWORD64(*)(DWORD64 GameObjectTypeArray, _Out_ DWORD64 * GameObjectType, DWORD ObjectTypeHash))(PathOfExile + 0xC31B50); // 3.18.0b
        auto InitGameObject         = (DWORD64(*)(GameObject * GameObject, DWORD64 * GameObjectType))(PathOfExile + 0x1671820);                                // 3.18.0b

        memset(this, 0, sizeof(GameObject));

        World world(9052, 1);

        DWORD64 GameObjectTypeArray = GetGameObjectTypeArray(&(world.GameObjectRegister));

        GetGameObjectType(GameObjectTypeArray, &ObjectTypeInfo, hash);

        if (ObjectTypeInfo != 0)
        {
            InitGameObject(this, &ObjectTypeInfo);
        }
    }

    QString GetMetadataId()
    {
        return QString::fromUtf16(*(char16_t **)(ObjectTypeInfo + 8));
    }

    QJsonObject GetComponents()
    {
        QJsonObject JsonObject;
        QJsonArray  JsonArray;

        for (Component **i = Components.Begin; i != Components.End; i++)
        {
            printf("0x%llx %s\n", *(DWORD64 *)((*(DWORD64 *)(*i)) + 0x88) - PathOfExile, (*i)->getName());
            JsonArray.append((*i)->getName());
        }

        JsonObject.insert(this->GetMetadataId(), JsonArray);
        return JsonObject;
    }
};

// ====================================================================================================

void fn(mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        mg_http_message *hm = (mg_http_message *)ev_data;

        if (mg_http_match_uri(hm, "/world"))
        {
            char hash16[16] = {};
            char seed[16]   = {};
            mg_http_get_var(&hm->query, "hash16", hash16, sizeof(hash16));
            mg_http_get_var(&hm->query, "seed", seed, sizeof(seed));

            World world(atoll(hash16), atoll(seed));
            printf("\n");
            printf("hash16:%s\n", hash16);
            printf("seed:%s\n", seed);
            printf("TileHash:0x%x\n", world.TileHash);
            printf("DoodadHash:0x%x\n", world.DoodadHash);
            qDebug() << world.WorldInfo.getId() << world.WorldInfo.getName();
            // world.terrain.show();

            QString header;
            header.append(QString("TileHash: %1\r\n").arg(world.TileHash));
            header.append(QString("DoodadHash: %1\r\n").arg(world.DoodadHash));
            header.append(QString("TerrainWidth: %1\r\n").arg((world.terrain.BytesPerRow * 2) + 1));
            header.append(QString("TerrainHeight: %1\r\n").arg(world.terrain.Height * 23));
            header.append(QString("WorldAreaId: %1\r\n").arg(world.WorldInfo.getId()));
            header.append(QString("WorldAreaName: %1\r\n").arg(world.WorldInfo.getName()));
            header.append(QString("RadarInfo: %1\r\n").arg(world.getRadarInfo()));

            mg_http_reply(c, 200, header.toLatin1().data(), world.getTerrain().data(), NULL);
        }
        else if (mg_http_match_uri(hm, "/ot"))
        {
            char hash[16] = {};
            mg_http_get_var(&hm->query, "hash", hash, sizeof(hash));

            printf("\n");
            qDebug() << "GameObjectHash:" << atoll(hash);

            GameObject obj(atoll(hash));

            if (obj.ObjectTypeInfo != 0)
            {
                QByteArray objInfo = QJsonDocument(obj.GetComponents()).toJson(QJsonDocument::Compact);
                qDebug() << objInfo;

                mg_http_reply(c, 200, NULL, objInfo.data(), NULL);
            }
            else
            {
                mg_http_reply(c, 500, NULL, "无效Hash", NULL);
            }
        }
        else
        {
            mg_http_reply(c, 500, NULL, "Invalid URI", NULL);
        }
    }
}

// ====================================================================================================

DWORD WINAPI DllThread(LPVOID hModule)
{
    mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://localhost:6112", fn, NULL);

    while (!GetAsyncKeyState(VK_END))
    {
        mg_mgr_poll(&mgr, 100);
    }

    mg_mgr_free(&mgr);

    FreeLibraryAndExitThread((HMODULE)hModule, 0);
}

// ====================================================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // AllocConsole
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

        printf("DLL_PROCESS_ATTACH\n");
        CreateThread(0, 0, DllThread, (LPVOID)hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        printf("DLL_PROCESS_DETACH\n");
        FreeConsole();
        break;
    }

    return TRUE;
}

// ====================================================================================================
