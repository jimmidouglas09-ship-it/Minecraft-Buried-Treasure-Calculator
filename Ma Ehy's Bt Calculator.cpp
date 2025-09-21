#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cfloat>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OverlayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
ATOM MyRegisterOverlayClass(HINSTANCE hInstance);
BOOL InitOverlay(HINSTANCE hInstance);
void UpdateOverlay();
void ShowOverlay();
void HideOverlay();
void SaveSettings();
void LoadSettings();
std::wstring GetSettingsFilePath();

// Structure to hold coordinates
struct Vec3 {
    int x, y, z;
};

// Structure for treasure locations with distance
struct TreasureLocation {
    int x, z;
    double distance;
};

// Global variables
Vec3 currentCoords = { 0, 0, 0 };
std::vector<TreasureLocation> nearestTreasures;
int currentHotkey = VK_F5;
int overlayToggleHotkey = VK_F6;
bool waitingForHotkey = false;
bool waitingForOverlayHotkey = false;

// Overlay variables
WCHAR szOverlayClass[] = L"TreasureOverlayClass";
HWND hOverlayWnd = NULL;
bool overlayVisible = false;
bool isDragging = false;
POINT dragOffset = { 0, 0 };

// Global variables for main window
HWND g_hMainWnd = NULL;
HHOOK g_hKeyboardHook = NULL;

// Buried treasure coordinates
std::vector<int> positiveCoords = {
    8, 24, 72, 88, 136, 152, 200, 216, 264, 280, 328, 344, 392, 408, 456, 472, 520, 536, 584, 600, 648, 664, 712, 728, 776, 792, 840, 856, 904, 920, 968, 984, 1032, 1048, 1096, 1112, 1160, 1176, 1224, 1240, 1288, 1304, 1352, 1368, 1416, 1432, 1480, 1496, 1544, 1560, 1608, 1624, 1672, 1688, 1736, 1752, 1800, 1816, 1864, 1880, 1928, 1944, 1992, 2008, 2056, 2072, 2120, 2136, 2184, 2200, 2248, 2264, 2312, 2328, 2376, 2392, 2440, 2456, 2504, 2520, 2568, 2584, 2632, 2648, 2696, 2712, 2760, 2776, 2824, 2840, 2888, 2904, 2952, 2968, 3016, 3032, 3080, 3096, 3144, 3160, 3208, 3224, 3272, 3288, 3336, 3352, 3400, 3416, 3464, 3480, 3528, 3544, 3592, 3608, 3656, 3672, 3720, 3736, 3784, 3800, 3848, 3864, 3912, 3928, 3976, 3992, 4040, 4056, 4104, 4120, 4168, 4184, 4232, 4248, 4296, 4312, 4360, 4376, 4424, 4440, 4488, 4504, 4552, 4568, 4616, 4632, 4680, 4696, 4744, 4760, 4808, 4824, 4872, 4888, 4936, 4952, 5000, 5016, 5064, 5080, 5128, 5144, 5192, 5208, 5256, 5272, 5320, 5336, 5384, 5400, 5448, 5464, 5512, 5528, 5576, 5592, 5640, 5656, 5704, 5720, 5768, 5784, 5832, 5848, 5896, 5912, 5960, 5976, 6024, 6040, 6088, 6104, 6152, 6168, 6216, 6232, 6280, 6296, 6344, 6360, 6408, 6424, 6472, 6488, 6536, 6552, 6600, 6616, 6664, 6680, 6728, 6744, 6792, 6808, 6856, 6872, 6920, 6936, 6984, 7000, 7048, 7064, 7112, 7128, 7176, 7192, 7240, 7256, 7304, 7320, 7368, 7384, 7432, 7448, 7496, 7512, 7560, 7576, 7624, 7640, 7688, 7704, 7752, 7768, 7816, 7832, 7880, 7896, 7944, 7960, 8008, 8024, 8072, 8088, 8136, 8152, 8200, 8216, 8264, 8280, 8328, 8344, 8392, 8408, 8456, 8472, 8520, 8536, 8584, 8600, 8648, 8664, 8712, 8728, 8776, 8792, 8840, 8856, 8904, 8920, 8968, 8984, 9032, 9048, 9096, 9112, 9160, 9176, 9224, 9240, 9288, 9304, 9352, 9368, 9416, 9432, 9480, 9496, 9544, 9560, 9608, 9624, 9672, 9688, 9736, 9752, 9800, 9816, 9864, 9880, 9928, 9944, 9992, 10008, 10056, 10072, 10120, 10136, 10184, 10200, 10248, 10264, 10312, 10328, 10376, 10392, 10440, 10456, 10504, 10520, 10568, 10584, 10632, 10648, 10696, 10712, 10760, 10776, 10824, 10840, 10888, 10904, 10952, 10968, 11016, 11032, 11080, 11096, 11144, 11160, 11208, 11224, 11272, 11288, 11336, 11352, 11400, 11416, 11464, 11480, 11528, 11544, 11592, 11608, 11656, 11672, 11720, 11736, 11784, 11800, 11848, 11864, 11912, 11928, 11976, 11992, 12040, 12056, 12104, 12120, 12168, 12184, 12232, 12248, 12296, 12312, 12360, 12376, 12424, 12440, 12488, 12504, 12552, 12568, 12616, 12632, 12680, 12696, 12744, 12760, 12808, 12824, 12872, 12888, 12936, 12952, 13000, 13016, 13064, 13080, 13128, 13144, 13192, 13208, 13256, 13272, 13320, 13336, 13384, 13400, 13448, 13464, 13512, 13528, 13576, 13592, 13640, 13656, 13704, 13720, 13768, 13784, 13832, 13848, 13896, 13912, 13960, 13976, 14024, 14040, 14088, 14104, 14152, 14168, 14216, 14232, 14280, 14296, 14344, 14360, 14408, 14424, 14472, 14488, 14536, 14552, 14600, 14616, 14664, 14680, 14728, 14744, 14792, 14808, 14856, 14872, 14920, 14936, 14984, 15000, 15048, 15064, 15112, 15128, 15176, 15192, 15240, 15256, 15304, 15320, 15368, 15384, 15432, 15448, 15496, 15512, 15560, 15576, 15624, 15640, 15688, 15704, 15752, 15768, 15816, 15832, 15880, 15896, 15944, 15960, 16008, 16024, 16072, 16088, 16136, 16152, 16200, 16216, 16264, 16280, 16328, 16344, 16392, 16408, 16456, 16472, 16520, 16536, 16584, 16600, 16648, 16664, 16712, 16728, 16776, 16792, 16840, 16856, 16904, 16920, 16968, 16984, 17032, 17048, 17096, 17112, 17160, 17176, 17224, 17240, 17288, 17304, 17352, 17368, 17416, 17432, 17480, 17496, 17544, 17560, 17608, 17624, 17672, 17688, 17736, 17752, 17800, 17816, 17864, 17880, 17928, 17944, 17992, 18008, 18056, 18072, 18120, 18136, 18184, 18200, 18248, 18264, 18312, 18328, 18376, 18392, 18440, 18456, 18504, 18520, 18568, 18584, 18632, 18648, 18696, 18712, 18760, 18776, 18824, 18840, 18888, 18904, 18952, 18968, 19016, 19032, 19080, 19096, 19144, 19160, 19208, 19224, 19272, 19288, 19336, 19352, 19400, 19416, 19464, 19480, 19528, 19544, 19592, 19608, 19656, 19672, 19720, 19736, 19784, 19800, 19848, 19864, 19912, 19928, 19976, 19992, 20040, 20056, 20104, 20120, 20168, 20184, 20232, 20248, 20296, 20312, 20360, 20376, 20424, 20440, 20488, 20504, 20552, 20568, 20616, 20632, 20680, 20696, 20744, 20760, 20808, 20824, 20872, 20888, 20936, 20952, 21000, 21016, 21064, 21080, 21128, 21144, 21192, 21208, 21256, 21272, 21320, 21336, 21384, 21400, 21448, 21464, 21512, 21528, 21576, 21592, 21640, 21656, 21704, 21720, 21768, 21784, 21832, 21848, 21896, 21912, 21960, 21976, 22024, 22040, 22088, 22104, 22152, 22168, 22216, 22232, 22280, 22296, 22344, 22360, 22408, 22424, 22472, 22488, 22536, 22552, 22600, 22616, 22664, 22680, 22728, 22744, 22792, 22808, 22856, 22872, 22920, 22936, 22984, 23000, 23048, 23064, 23112, 23128, 23176, 23192, 23240, 23256, 23304, 23320, 23368, 23384, 23432, 23448, 23496, 23512, 23560, 23576, 23624, 23640, 23688, 23704, 23752, 23768, 23816, 23832, 23880, 23896, 23944, 23960, 24008, 24024, 24072, 24088, 24136, 24152, 24200, 24216, 24264, 24280, 24328, 24344, 24392, 24408, 24456, 24472, 24520, 24536, 24584, 24600, 24648, 24664, 24712, 24728, 24776, 24792, 24840, 24856, 24904, 24920, 24968, 24984, 25032, 25048, 25096, 25112, 25160, 25176, 25224, 25240, 25288, 25304, 25352, 25368, 25416, 25432, 25480, 25496, 25544, 25560, 25608, 25624, 25672, 25688, 25736, 25752, 25800, 25816, 25864, 25880, 25928, 25944, 25992, 26008, 26056, 26072, 26120, 26136, 26184, 26200, 26248, 26264, 26312, 26328, 26376, 26392, 26440, 26456, 26504, 26520, 26568, 26584, 26632, 26648, 26696, 26712, 26760, 26776, 26824, 26840, 26888, 26904, 26952, 26968, 27016, 27032, 27080, 27096, 27144, 27160, 27208, 27224, 27272, 27288, 27336, 27352, 27400, 27416, 27464, 27480, 27528, 27544, 27592, 27608, 27656, 27672, 27720, 27736, 27784, 27800, 27848, 27864, 27912, 27928, 27976, 27992, 28040, 28056, 28104, 28120, 28168, 28184, 28232, 28248, 28296, 28312, 28360, 28376, 28424, 28440, 28488, 28504, 28552, 28568, 28616, 28632, 28680, 28696, 28744, 28760, 28808, 28824, 28872, 28888, 28936, 28952, 29000, 29016, 29064, 29080, 29128, 29144, 29192, 29208, 29256, 29272, 29320, 29336, 29384, 29400, 29448, 29464, 29512, 29528, 29576, 29592, 29640, 29656, 29704, 29720, 29768, 29784, 29832, 29848, 29896, 29912, 29960, 29976, 30024, 30040, 30088, 30104, 30152, 30168, 30216, 30232, 30280, 30296, 30344, 30360, 30408, 30424, 30472, 30488, 30536, 30552, 30600, 30616, 30664, 30680, 30728, 30744, 30792, 30808, 30856, 30872, 30920, 30936, 30984, 31000, 31048, 31064, 31112, 31128, 31176, 31192, 31240, 31256, 31304, 31320, 31368, 31384, 31432, 31448, 31496, 31512, 31560, 31576, 31624, 31640, 31688, 31704, 31752, 31768, 31816, 31832, 31880, 31896, 31944, 31960, 32008, 32024, 32072, 32088, 32136, 32152, 32200, 32216, 32264, 32280, 32328, 32344, 32392, 32408, 32456, 32472, 32520, 32536, 32584, 32600, 32648, 32664, 32712, 32728, 32776, 32792, 32840, 32856, 32904, 32920, 32968, 32984, 33032, 33048, 33096, 33112, 33160, 33176, 33224, 33240, 33288, 33304, 33352, 33368, 33416, 33432, 33480, 33496, 33544, 33560, 33608, 33624, 33672, 33688, 33736, 33752, 33800, 33816, 33864, 33880, 33928, 33944, 33992, 34008, 34056, 34072, 34120, 34136, 34184, 34200, 34248, 34264, 34312, 34328, 34376, 34392, 34440, 34456, 34504, 34520, 34568, 34584, 34632, 34648, 34696, 34712, 34760, 34776, 34824, 34840, 34888, 34904, 34952, 34968, 35016, 35032, 35080, 35096, 35144, 35160, 35208, 35224, 35272, 35288, 35336, 35352, 35400, 35416, 35464, 35480, 35528, 35544, 35592, 35608, 35656, 35672, 35720, 35736, 35784, 35800, 35848, 35864, 35912, 35928, 35976, 35992, 36040, 36056, 36104, 36120, 36168, 36184, 36232, 36248, 36296, 36312, 36360, 36376, 36424, 36440, 36488, 36504, 36552, 36568, 36616, 36632, 36680, 36696, 36744, 36760, 36808, 36824, 36872, 36888, 36936, 36952, 37000, 37016, 37064, 37080, 37128, 37144, 37192, 37208, 37256, 37272, 37320, 37336, 37384, 37400, 37448, 37464, 37512, 37528, 37576, 37592, 37640, 37656, 37704, 37720, 37768, 37784, 37832, 37848, 37896, 37912, 37960, 37976, 38024, 38040, 38088, 38104, 38152, 38168, 38216, 38232, 38280, 38296, 38344, 38360, 38408, 38424, 38472, 38488, 38536, 38552, 38600, 38616, 38664, 38680, 38728, 38744, 38792, 38808, 38856, 38872, 38920, 38936, 38984, 39000, 39048, 39064, 39112, 39128, 39176, 39192, 39240, 39256, 39304, 39320, 39368, 39384, 39432, 39448, 39496, 39512, 39560, 39576, 39624, 39640, 39688, 39704, 39752, 39768, 39816, 39832, 39880, 39896, 39944, 39960, 40008, 40024, 40072, 40088, 40136, 40152, 40200, 40216, 40264, 40280, 40328, 40344, 40392, 40408, 40456, 40472, 40520, 40536, 40584, 40600, 40648, 40664, 40712, 40728, 40776, 40792, 40840, 40856, 40904, 40920, 40968, 40984, 41032, 41048, 41096, 41112, 41160, 41176, 41224, 41240, 41288, 41304, 41352, 41368, 41416, 41432, 41480, 41496, 41544, 41560, 41608, 41624, 41672, 41688, 41736, 41752, 41800, 41816, 41864, 41880, 41928, 41944, 41992, 42008, 42056, 42072, 42120, 42136, 42184, 42200, 42248, 42264, 42312, 42328, 42376, 42392, 42440, 42456, 42504, 42520, 42568, 42584, 42632, 42648, 42696, 42712, 42760, 42776, 42824, 42840, 42888, 42904, 42952, 42968, 43016, 43032, 43080, 43096, 43144, 43160, 43208, 43224, 43272, 43288, 43336, 43352, 43400, 43416, 43464, 43480, 43528, 43544, 43592, 43608, 43656, 43672, 43720, 43736, 43784, 43800, 43848, 43864, 43912, 43928, 43976, 43992, 44040, 44056, 44104, 44120, 44168, 44184, 44232, 44248, 44296, 44312, 44360, 44376, 44424, 44440, 44488, 44504, 44552, 44568, 44616, 44632, 44680, 44696, 44744, 44760, 44808, 44824, 44872, 44888, 44936, 44952, 45000, 45016, 45064, 45080, 45128, 45144, 45192, 45208, 45256, 45272, 45320, 45336, 45384, 45400, 45448, 45464, 45512, 45528, 45576, 45592, 45640, 45656, 45704, 45720, 45768, 45784, 45832, 45848, 45896, 45912, 45960, 45976, 46024, 46040, 46088, 46104, 46152, 46168, 46216, 46232, 46280, 46296, 46344, 46360, 46408, 46424, 46472, 46488, 46536, 46552, 46600, 46616, 46664, 46680, 46728, 46744, 46792, 46808, 46856, 46872, 46920, 46936, 46984, 47000, 47048, 47064, 47112, 47128, 47176, 47192, 47240, 47256, 47304, 47320, 47368, 47384, 47432, 47448, 47496, 47512, 47560, 47576, 47624, 47640, 47688, 47704, 47752, 47768, 47816, 47832, 47880, 47896, 47944, 47960, 48008, 48024, 48072, 48088, 48136, 48152, 48200, 48216, 48264, 48280, 48328, 48344, 48392, 48408, 48456, 48472, 48520, 48536, 48584, 48600, 48648, 48664, 48712, 48728, 48776, 48792, 48840, 48856, 48904, 48920, 48968, 48984, 49032, 49048, 49096, 49112, 49160, 49176, 49224, 49240, 49288, 49304, 49352, 49368, 49416, 49432, 49480, 49496, 49544, 49560, 49608, 49624, 49672, 49688, 49736, 49752, 49800, 49816, 49864, 49880, 49928, 49944, 49992
};

std::vector<int> negativeCoords = {
    -49976, -49960, -49912, -49896, -49848, -49832, -49784, -49768, -49720, -49704, -49656, -49640, -49592, -49576, -49528, -49512, -49464, -49448, -49400, -49384, -49336, -49320, -49272, -49256, -49208, -49192, -49144, -49128, -49080, -49064, -49016, -49000, -48952, -48936, -48888, -48872, -48824, -48808, -48760, -48744, -48696, -48680, -48632, -48616, -48568, -48552, -48504, -48488, -48440, -48424, -48376, -48360, -48312, -48296, -48248, -48232, -48184, -48168, -48120, -48104, -48056, -48040, -47992, -47976, -47928, -47912, -47864, -47848, -47800, -47784, -47736, -47720, -47672, -47656, -47608, -47592, -47544, -47528, -47480, -47464, -47416, -47400, -47352, -47336, -47288, -47272, -47224, -47208, -47160, -47144, -47096, -47080, -47032, -47016, -46968, -46952, -46904, -46888, -46840, -46824, -46776, -46760, -46712, -46696, -46648, -46632, -46584, -46568, -46520, -46504, -46456, -46440, -46392, -46376, -46328, -46312, -46264, -46248, -46200, -46184, -46136, -46120, -46072, -46056, -46008, -45992, -45944, -45928, -45880, -45864, -45816, -45800, -45752, -45736, -45688, -45672, -45624, -45608, -45560, -45544, -45496, -45480, -45432, -45416, -45368, -45352, -45304, -45288, -45240, -45224, -45176, -45160, -45112, -45096, -45048, -45032, -44984, -44968, -44920, -44904, -44856, -44840, -44792, -44776, -44728, -44712, -44664, -44648, -44600, -44584, -44536, -44520, -44472, -44456, -44408, -44392, -44344, -44328, -44280, -44264, -44216, -44200, -44152, -44136, -44088, -44072, -44024, -44008, -43960, -43944, -43896, -43880, -43832, -43816, -43768, -43752, -43704, -43688, -43640, -43624, -43576, -43560, -43512, -43496, -43448, -43432, -43384, -43368, -43320, -43304, -43256, -43240, -43192, -43176, -43128, -43112, -43064, -43048, -43000, -42984, -42936, -42920, -42872, -42856, -42808, -42792, -42744, -42728, -42680, -42664, -42616, -42600, -42552, -42536, -42488, -42472, -42424, -42408, -42360, -42344, -42296, -42280, -42232, -42216, -42168, -42152, -42104, -42088, -42040, -42024, -41976, -41960, -41912, -41896, -41848, -41832, -41784, -41768, -41720, -41704, -41656, -41640, -41592, -41576, -41528, -41512, -41464, -41448, -41400, -41384, -41336, -41320, -41272, -41256, -41208, -41192, -41144, -41128, -41080, -41064, -41016, -41000, -40952, -40936, -40888, -40872, -40824, -40808, -40760, -40744, -40696, -40680, -40632, -40616, -40568, -40552, -40504, -40488, -40440, -40424, -40376, -40360, -40312, -40296, -40248, -40232, -40184, -40168, -40120, -40104, -40056, -40040, -39992, -39976, -39928, -39912, -39864, -39848, -39800, -39784, -39736, -39720, -39672, -39656, -39608, -39592, -39544, -39528, -39480, -39464, -39416, -39400, -39352, -39336, -39288, -39272, -39224, -39208, -39160, -39144, -39096, -39080, -39032, -39016, -38968, -38952, -38904, -38888, -38840, -38824, -38776, -38760, -38712, -38696, -38648, -38632, -38584, -38568, -38520, -38504, -38456, -38440, -38392, -38376, -38328, -38312, -38264, -38248, -38200, -38184, -38136, -38120, -38072, -38056, -38008, -37992, -37944, -37928, -37880, -37864, -37816, -37800, -37752, -37736, -37688, -37672, -37624, -37608, -37560, -37544, -37496, -37480, -37432, -37416, -37368, -37352, -37304, -37288, -37240, -37224, -37176, -37160, -37112, -37096, -37048, -37032, -36984, -36968, -36920, -36904, -36856, -36840, -36792, -36776, -36728, -36712, -36664, -36648, -36600, -36584, -36536, -36520, -36472, -36456, -36408, -36392, -36344, -36328, -36280, -36264, -36216, -36200, -36152, -36136, -36088, -36072, -36024, -36008, -35960, -35944, -35896, -35880, -35832, -35816, -35768, -35752, -35704, -35688, -35640, -35624, -35576, -35560, -35512, -35496, -35448, -35432, -35384, -35368, -35320, -35304, -35256, -35240, -35192, -35176, -35128, -35112, -35064, -35048, -35000, -34984, -34936, -34920, -34872, -34856, -34808, -34792, -34744, -34728, -34680, -34664, -34616, -34600, -34552, -34536, -34488, -34472, -34424, -34408, -34360, -34344, -34296, -34280, -34232, -34216, -34168, -34152, -34104, -34088, -34040, -34024, -33976, -33960, -33912, -33896, -33848, -33832, -33784, -33768, -33720, -33704, -33656, -33640, -33592, -33576, -33528, -33512, -33464, -33448, -33400, -33384, -33336, -33320, -33272, -33256, -33208, -33192, -33144, -33128, -33080, -33064, -33016, -33000, -32952, -32936, -32888, -32872, -32824, -32808, -32760, -32744, -32696, -32680, -32632, -32616, -32568, -32552, -32504, -32488, -32440, -32424, -32376, -32360, -32312, -32296, -32248, -32232, -32184, -32168, -32120, -32104, -32056, -32040, -31992, -31976, -31928, -31912, -31864, -31848, -31800, -31784, -31736, -31720, -31672, -31656, -31608, -31592, -31544, -31528, -31480, -31464, -31416, -31400, -31352, -31336, -31288, -31272, -31224, -31208, -31160, -31144, -31096, -31080, -31032, -31016, -30968, -30952, -30904, -30888, -30840, -30824, -30776, -30760, -30712, -30696, -30648, -30632, -30584, -30568, -30520, -30504, -30456, -30440, -30392, -30376, -30328, -30312, -30264, -30248, -30200, -30184, -30136, -30120, -30072, -30056, -30008, -29992, -29944, -29928, -29880, -29864, -29816, -29800, -29752, -29736, -29688, -29672, -29624, -29608, -29560, -29544, -29496, -29480, -29432, -29416, -29368, -29352, -29304, -29288, -29240, -29224, -29176, -29160, -29112, -29096, -29048, -29032, -28984, -28968, -28920, -28904, -28856, -28840, -28792, -28776, -28728, -28712, -28664, -28648, -28600, -28584, -28536, -28520, -28472, -28456, -28408, -28392, -28344, -28328, -28280, -28264, -28216, -28200, -28152, -28136, -28088, -28072, -28024, -28008, -27960, -27944, -27896, -27880, -27832, -27816, -27768, -27752, -27704, -27688, -27640, -27624, -27576, -27560, -27512, -27496, -27448, -27432, -27384, -27368, -27320, -27304, -27256, -27240, -27192, -27176, -27128, -27112, -27064, -27048, -27000, -26984, -26936, -26920, -26872, -26856, -26808, -26792, -26744, -26728, -26680, -26664, -26616, -26600, -26552, -26536, -26488, -26472, -26424, -26408, -26360, -26344, -26296, -26280, -26232, -26216, -26168, -26152, -26104, -26088, -26040, -26024, -25976, -25960, -25912, -25896, -25848, -25832, -25784, -25768, -25720, -25704, -25656, -25640, -25592, -25576, -25528, -25512, -25464, -25448, -25400, -25384, -25336, -25320, -25272, -25256, -25208, -25192, -25144, -25128, -25080, -25064, -25016, -25000, -24952, -24936, -24888, -24872, -24824, -24808, -24760, -24744, -24696, -24680, -24632, -24616, -24568, -24552, -24504, -24488, -24440, -24424, -24376, -24360, -24312, -24296, -24248, -24232, -24184, -24168, -24120, -24104, -24056, -24040, -23992, -23976, -23928, -23912, -23864, -23848, -23800, -23784, -23736, -23720, -23672, -23656, -23608, -23592, -23544, -23528, -23480, -23464, -23416, -23400, -23352, -23336, -23288, -23272, -23224, -23208, -23160, -23144, -23096, -23080, -23032, -23016, -22968, -22952, -22904, -22888, -22840, -22824, -22776, -22760, -22712, -22696, -22648, -22632, -22584, -22568, -22520, -22504, -22456, -22440, -22392, -22376, -22328, -22312, -22264, -22248, -22200, -22184, -22136, -22120, -22072, -22056, -22008, -21992, -21944, -21928, -21880, -21864, -21816, -21800, -21752, -21736, -21688, -21672, -21624, -21608, -21560, -21544, -21496, -21480, -21432, -21416, -21368, -21352, -21304, -21288, -21240, -21224, -21176, -21160, -21112, -21096, -21048, -21032, -20984, -20968, -20920, -20904, -20856, -20840, -20792, -20776, -20728, -20712, -20664, -20648, -20600, -20584, -20536, -20520, -20472, -20456, -20408, -20392, -20344, -20328, -20280, -20264, -20216, -20200, -20152, -20136, -20088, -20072, -20024, -20008, -19960, -19944, -19896, -19880, -19832, -19816, -19768, -19752, -19704, -19688, -19640, -19624, -19576, -19560, -19512, -19496, -19448, -19432, -19384, -19368, -19320, -19304, -19256, -19240, -19192, -19176, -19128, -19112, -19064, -19048, -19000, -18984, -18936, -18920, -18872, -18856, -18808, -18792, -18744, -18728, -18680, -18664, -18616, -18600, -18552, -18536, -18488, -18472, -18424, -18408, -18360, -18344, -18296, -18280, -18232, -18216, -18168, -18152, -18104, -18088, -18040, -18024, -17976, -17960, -17912, -17896, -17848, -17832, -17784, -17768, -17720, -17704, -17656, -17640, -17592, -17576, -17528, -17512, -17464, -17448, -17400, -17384, -17336, -17320, -17272, -17256, -17208, -17192, -17144, -17128, -17080, -17064, -17016, -17000, -16952, -16936, -16888, -16872, -16824, -16808, -16760, -16744, -16696, -16680, -16632, -16616, -16568, -16552, -16504, -16488, -16440, -16424, -16376, -16360, -16312, -16296, -16248, -16232, -16184, -16168, -16120, -16104, -16056, -16040, -15992, -15976, -15928, -15912, -15864, -15848, -15800, -15784, -15736, -15720, -15672, -15656, -15608, -15592, -15544, -15528, -15480, -15464, -15416, -15400, -15352, -15336, -15288, -15272, -15224, -15208, -15160, -15144, -15096, -15080, -15032, -15016, -14968, -14952, -14904, -14888, -14840, -14824, -14776, -14760, -14712, -14696, -14648, -14632, -14584, -14568, -14520, -14504, -14456, -14440, -14392, -14376, -14328, -14312, -14264, -14248, -14200, -14184, -14136, -14120, -14072, -14056, -14008, -13992, -13944, -13928, -13880, -13864, -13816, -13800, -13752, -13736, -13688, -13672, -13624, -13608, -13560, -13544, -13496, -13480, -13432, -13416, -13368, -13352, -13304, -13288, -13240, -13224, -13176, -13160, -13112, -13096, -13048, -13032, -12984, -12968, -12920, -12904, -12856, -12840, -12792, -12776, -12728, -12712, -12664, -12648, -12600, -12584, -12536, -12520, -12472, -12456, -12408, -12392, -12344, -12328, -12280, -12264, -12216, -12200, -12152, -12136, -12088, -12072, -12024, -12008, -11960, -11944, -11896, -11880, -11832, -11816, -11768, -11752, -11704, -11688, -11640, -11624, -11576, -11560, -11512, -11496, -11448, -11432, -11384, -11368, -11320, -11304, -11256, -11240, -11192, -11176, -11128, -11112, -11064, -11048, -11000, -10984, -10936, -10920, -10872, -10856, -10808, -10792, -10744, -10728, -10680, -10664, -10616, -10600, -10552, -10536, -10488, -10472, -10424, -10408, -10360, -10344, -10296, -10280, -10232, -10216, -10168, -10152, -10104, -10088, -10040, -10024, -9976, -9960, -9912, -9896, -9848, -9832, -9784, -9768, -9720, -9704, -9656, -9640, -9592, -9576, -9528, -9512, -9464, -9448, -9400, -9384, -9336, -9320, -9272, -9256, -9208, -9192, -9144, -9128, -9080, -9064, -9016, -9000, -8952, -8936, -8888, -8872, -8824, -8808, -8760, -8744, -8696, -8680, -8632, -8616, -8568, -8552, -8504, -8488, -8440, -8424, -8376, -8360, -8312, -8296, -8248, -8232, -8184, -8168, -8120, -8104, -8056, -8040, -7992, -7976, -7928, -7912, -7864, -7848, -7800, -7784, -7736, -7720, -7672, -7656, -7608, -7592, -7544, -7528, -7480, -7464, -7416, -7400, -7352, -7336, -7288, -7272, -7224, -7208, -7160, -7144, -7096, -7080, -7032, -7016, -6968, -6952, -6904, -6888, -6840, -6824, -6776, -6760, -6712, -6696, -6648, -6632, -6584, -6568, -6520, -6504, -6456, -6440, -6392, -6376, -6328, -6312, -6264, -6248, -6200, -6184, -6136, -6120, -6072, -6056, -6008, -5992, -5944, -5928, -5880, -5864, -5816, -5800, -5752, -5736, -5688, -5672, -5624, -5608, -5560, -5544, -5496, -5480, -5432, -5416, -5368, -5352, -5304, -5288, -5240, -5224, -5176, -5160, -5112, -5096, -5048, -5032, -4984, -4968, -4920, -4904, -4856, -4840, -4792, -4776, -4728, -4712, -4664, -4648, -4600, -4584, -4536, -4520, -4472, -4456, -4408, -4392, -4344, -4328, -4280, -4264, -4216, -4200, -4152, -4136, -4088, -4072, -4024, -4008, -3960, -3944, -3896, -3880, -3832, -3816, -3768, -3752, -3704, -3688, -3640, -3624, -3576, -3560, -3512, -3496, -3448, -3432, -3384, -3368, -3320, -3304, -3256, -3240, -3192, -3176, -3128, -3112, -3064, -3048, -3000, -2984, -2936, -2920, -2872, -2856, -2808, -2792, -2744, -2728, -2680, -2664, -2616, -2600, -2552, -2536, -2488, -2472, -2424, -2408, -2360, -2344, -2296, -2280, -2232, -2216, -2168, -2152, -2104, -2088, -2040, -2024, -1976, -1960, -1912, -1896, -1848, -1832, -1784, -1768, -1720, -1704, -1656, -1640, -1592, -1576, -1528, -1512, -1464, -1448, -1400, -1384, -1336, -1320, -1272, -1256, -1208, -1192, -1144, -1128, -1080, -1064, -1016, -1000, -952, -936, -888, -872, -824, -808, -760, -744, -696, -680, -632, -616, -568, -552, -504, -488, -440, -424, -376, -360, -312, -296, -248, -232, -184, -168, -120, -104, -56, -40
};

// Settings functions
std::wstring GetSettingsFilePath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        path = path.substr(0, pos + 1);
    }
    return path + L"treasure_finder_settings.ini";
}

void SaveSettings() {
    std::wstring settingsFile = GetSettingsFilePath();

    WritePrivateProfileString(L"Hotkeys", L"FindTreasure", std::to_wstring(currentHotkey).c_str(), settingsFile.c_str());
    WritePrivateProfileString(L"Hotkeys", L"ToggleOverlay", std::to_wstring(overlayToggleHotkey).c_str(), settingsFile.c_str());

    // Save overlay position and visibility
    if (hOverlayWnd) {
        RECT rect;
        GetWindowRect(hOverlayWnd, &rect);
        WritePrivateProfileString(L"Overlay", L"X", std::to_wstring(rect.left).c_str(), settingsFile.c_str());
        WritePrivateProfileString(L"Overlay", L"Y", std::to_wstring(rect.top).c_str(), settingsFile.c_str());
        WritePrivateProfileString(L"Overlay", L"Visible", overlayVisible ? L"1" : L"0", settingsFile.c_str());
    }
}

void LoadSettings() {
    std::wstring settingsFile = GetSettingsFilePath();

    currentHotkey = GetPrivateProfileInt(L"Hotkeys", L"FindTreasure", VK_F5, settingsFile.c_str());
    overlayToggleHotkey = GetPrivateProfileInt(L"Hotkeys", L"ToggleOverlay", VK_F6, settingsFile.c_str());

    // Validate hotkeys are different
    if (currentHotkey == overlayToggleHotkey) {
        overlayToggleHotkey = VK_F6;
        currentHotkey = VK_F5;
        SaveSettings(); // Save corrected values
    }
}

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

        if (waitingForHotkey || waitingForOverlayHotkey) {
            int newKey = (int)pKeyboard->vkCode;

            if (newKey != VK_ESCAPE) {
                if (waitingForHotkey && newKey != overlayToggleHotkey) {
                    UnregisterHotKey(g_hMainWnd, 1);
                    currentHotkey = newKey;
                    RegisterHotKey(g_hMainWnd, 1, 0, currentHotkey);
                    SaveSettings();
                    waitingForHotkey = false;
                }
                else if (waitingForOverlayHotkey && newKey != currentHotkey) {
                    UnregisterHotKey(g_hMainWnd, 2);
                    overlayToggleHotkey = newKey;
                    RegisterHotKey(g_hMainWnd, 2, 0, overlayToggleHotkey);
                    SaveSettings();
                    waitingForOverlayHotkey = false;
                }
            }
            else {
                waitingForHotkey = false;
                waitingForOverlayHotkey = false;
            }

            if (g_hMainWnd) {
                InvalidateRect(g_hMainWnd, NULL, TRUE);
            }

            return 1; // Consume the key event
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Coordinate reading functions
std::unique_ptr<Bitmap> BitmapFromHWND(HWND hwnd) {
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    RECT rc; GetWindowRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);
    PrintWindow(hwnd, memDC, PW_RENDERFULLCONTENT);
    auto pBitmap = std::make_unique<Bitmap>(hBitmap, nullptr);
    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
    return pBitmap;
}

int GetShownCoordinates(HWND hwnd, Vec3* coordinates) {
    auto pBitmap = BitmapFromHWND(hwnd);
    int width = pBitmap->GetWidth();
    int height = pBitmap->GetHeight();
    int searchWidth = width;
    int searchHeight = height / 3;
    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0, 0, searchWidth, searchHeight);
    pBitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
    int startTextX = 0, startTextY = 0, streak = 0;
    int stride = bitmapData.Stride / sizeof(ARGB);
    ARGB* pixels = static_cast<ARGB*>(bitmapData.Scan0);

    for (int y = 30; y < searchHeight; y++) {
        for (int x = 8; x < searchWidth; x++) {
            if (pixels[y * stride + x] == 0xFFFFFFFF) {
                if (!startTextX) { startTextX = x; startTextY = y; }
                streak++;
            }
            else if (streak < 4) streak = 0;
            else if (streak >= 4) break;
        }
        if (streak >= 4) break;
    }

    if (streak < 4) return 0;
    int scale = streak / 4;
    startTextX += 44 * scale;
    int coords[3] = { 0, 0, 0 };
    int index = 0;
    bool isSigned = false;

    while (startTextX < searchWidth) {
        unsigned int columnMask = 0;
        for (int dy = 0; dy < 7; dy++) {
            columnMask <<= 1;
            if (pixels[(startTextY + dy * scale) * stride + startTextX] == 0xFFFFFFFF)
                columnMask |= 1;
        }

        int digit = -1;
        switch (columnMask) {
        case 0b0111110: digit = 0; break;
        case 0b0000001: digit = 1; break;
        case 0b0100011: digit = 2; break;
        case 0b0100010: digit = 3; break;
        case 0b0001100: digit = 4; break;
        case 0b1110010: digit = 5; break;
        case 0b0011110: digit = 6; break;
        case 0b1100000: digit = 7; break;
        case 0b0110110: digit = 8; break;
        case 0b0110000: digit = 9; break;
        case 0b0001000: isSigned = true; break;
        case 0b0000011:
            if (isSigned) coords[index] *= -1;
            if (++index > 2) break;
            isSigned = false;
            break;
        default:
            if (index >= 2) break;
            if (isSigned) coords[index] *= -1;
            break;
        }

        if (digit != -1)
            coords[index] = coords[index] * 10 + digit;
        startTextX += 6 * scale;
    }

    if (isSigned && index <= 2) {
        coords[index] *= -1;
    }

    pBitmap->UnlockBits(&bitmapData);
    coordinates->x = coords[0];
    coordinates->y = coords[1];
    coordinates->z = coords[2];
    return 1;
}

// Find closest treasure coordinate for a given player coordinate
int FindClosestTreasureCoord(int playerCoord) {
    int closest = 0;
    double minDistance = DBL_MAX;

    // Check positive coordinates
    for (int coord : positiveCoords) {
        double distance = abs(coord - playerCoord);
        if (distance < minDistance) {
            minDistance = distance;
            closest = coord;
        }
    }

    // Check negative coordinates
    for (int coord : negativeCoords) {
        double distance = abs(coord - playerCoord);
        if (distance < minDistance) {
            minDistance = distance;
            closest = coord;
        }
    }

    return closest;
}

// Calculate distance between two points
double CalculateDistance(int x1, int z1, int x2, int z2) {
    return sqrt((x2 - x1) * (x2 - x1) + (z2 - z1) * (z2 - z1));
}

// Find nearest buried treasures
void FindNearestTreasures(int playerX, int playerZ) {
    nearestTreasures.clear();

    // Get closest X and Z coordinates
    int closestX = FindClosestTreasureCoord(playerX);
    int closestZ = FindClosestTreasureCoord(playerZ);

    // Find several nearby treasure coordinates for both X and Z
    std::vector<int> nearbyX, nearbyZ;

    // Add closest and nearby X coordinates
    for (int coord : positiveCoords) {
        if (abs(coord - playerX) <= abs(closestX - playerX) + 200) {
            nearbyX.push_back(coord);
        }
    }
    for (int coord : negativeCoords) {
        if (abs(coord - playerX) <= abs(closestX - playerX) + 200) {
            nearbyX.push_back(coord);
        }
    }

    // Add closest and nearby Z coordinates
    for (int coord : positiveCoords) {
        if (abs(coord - playerZ) <= abs(closestZ - playerZ) + 200) {
            nearbyZ.push_back(coord);
        }
    }
    for (int coord : negativeCoords) {
        if (abs(coord - playerZ) <= abs(closestZ - playerZ) + 200) {
            nearbyZ.push_back(coord);
        }
    }

    // Generate all combinations and calculate distances
    for (int x : nearbyX) {
        for (int z : nearbyZ) {
            TreasureLocation treasure;
            treasure.x = x;
            treasure.z = z;
            treasure.distance = CalculateDistance(playerX, playerZ, x, z);
            nearestTreasures.push_back(treasure);
        }
    }

    // Sort by distance
    std::sort(nearestTreasures.begin(), nearestTreasures.end(),
        [](const TreasureLocation& a, const TreasureLocation& b) {
            return a.distance < b.distance;
        });

    // Keep only the closest 10
    if (nearestTreasures.size() > 10) {
        nearestTreasures.resize(10);
    }
}

// Overlay window functions
ATOM MyRegisterOverlayClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = OverlayProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszClassName = szOverlayClass;
    return RegisterClassExW(&wcex);
}

BOOL InitOverlay(HINSTANCE hInstance) {
    std::wstring settingsFile = GetSettingsFilePath();

    // Load overlay position from settings, or use default
    int x = GetPrivateProfileInt(L"Overlay", L"X", GetSystemMetrics(SM_CXSCREEN) - 420, settingsFile.c_str());
    int y = GetPrivateProfileInt(L"Overlay", L"Y", 20, settingsFile.c_str());
    overlayVisible = GetPrivateProfileInt(L"Overlay", L"Visible", 0, settingsFile.c_str()) != 0;

    int overlayWidth = 400;
    int overlayHeight = 280;

    hOverlayWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        szOverlayClass,
        L"Treasure Overlay",
        WS_POPUP,
        x, y, overlayWidth, overlayHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hOverlayWnd) {
        return FALSE;
    }

    // Make window semi-transparent with black as transparent color
    SetLayeredWindowAttributes(hOverlayWnd, RGB(0, 0, 0), 220, LWA_COLORKEY | LWA_ALPHA);

    // Show overlay based on saved settings
    if (overlayVisible) {
        ShowWindow(hOverlayWnd, SW_SHOW);
    }

    return TRUE;
}

void UpdateOverlay() {
    if (hOverlayWnd && overlayVisible) {
        InvalidateRect(hOverlayWnd, NULL, TRUE);
    }
}

void ShowOverlay() {
    if (hOverlayWnd && !overlayVisible) {
        ShowWindow(hOverlayWnd, SW_SHOW);
        SetWindowPos(hOverlayWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        overlayVisible = true;
        UpdateOverlay();
        SaveSettings(); // Save the new visibility state
    }
}

void HideOverlay() {
    if (hOverlayWnd && overlayVisible) {
        ShowWindow(hOverlayWnd, SW_HIDE);
        overlayVisible = false;
        isDragging = false;
        SaveSettings(); // Save the new visibility state
    }
}

void ToggleOverlay() {
    if (overlayVisible) {
        HideOverlay();
    }
    else {
        ShowOverlay();
    }
}

LRESULT CALLBACK OverlayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN:
    {
        // Start dragging
        isDragging = true;
        SetCapture(hWnd);

        POINT cursorPos;
        GetCursorPos(&cursorPos);

        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);

        dragOffset.x = cursorPos.x - windowRect.left;
        dragOffset.y = cursorPos.y - windowRect.top;
    }
    break;

    case WM_LBUTTONUP:
    {
        // Stop dragging and save position
        if (isDragging) {
            isDragging = false;
            ReleaseCapture();
            SaveSettings(); // Save new position
        }
    }
    break;

    case WM_MOUSEMOVE:
    {
        if (isDragging) {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            int newX = cursorPos.x - dragOffset.x;
            int newY = cursorPos.y - dragOffset.y;

            // Keep window on screen
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            RECT windowRect;
            GetWindowRect(hWnd, &windowRect);
            int windowWidth = windowRect.right - windowRect.left;
            int windowHeight = windowRect.bottom - windowRect.top;

            if (newX < 0) newX = 0;
            if (newY < 0) newY = 0;
            if (newX + windowWidth > screenWidth) newX = screenWidth - windowWidth;
            if (newY + windowHeight > screenHeight) newY = screenHeight - windowHeight;

            SetWindowPos(hWnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    break;

    case WM_MOVE:
    {
        // Save position when window is moved (either by dragging or other means)
        SaveSettings();
    }
    break;

    case WM_SETCURSOR:
    {
        // Show move cursor when hovering over window
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return TRUE;
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // Dark background with transparency
        graphics.Clear(Color(240, 15, 20, 35));

        // Draw border with grip indicator
        Pen borderPen(Color(255, 100, 150, 200), 2);
        RECT rect;
        GetClientRect(hWnd, &rect);
        graphics.DrawRectangle(&borderPen, 1, 1, rect.right - 2, rect.bottom - 2);

        // Draw grip indicator in top-left corner
        SolidBrush gripBrush(Color(255, 100, 150, 200));
        graphics.FillRectangle(&gripBrush, 3, 3, 8, 8);

        // Fonts
        FontFamily fontFamily(L"Consolas");
        Font headerFont(&fontFamily, 15, FontStyleBold, UnitPixel);
        Font coordFont(&fontFamily, 13, FontStyleBold, UnitPixel);
        Font infoFont(&fontFamily, 12, FontStyleRegular, UnitPixel);
        Font smallFont(&fontFamily, 11, FontStyleRegular, UnitPixel);

        // Colors
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        SolidBrush lightGrayBrush(Color(255, 200, 200, 200));
        SolidBrush greenBrush(Color(255, 120, 255, 120));
        SolidBrush yellowBrush(Color(255, 255, 220, 100));
        SolidBrush orangeBrush(Color(255, 255, 165, 0));
        SolidBrush cyanBrush(Color(255, 100, 200, 255));

        int y = 12;
        int x = 18;

        // Title
        graphics.DrawString(L"Buried Treasure Finder", -1, &headerFont,
            PointF((REAL)x, (REAL)y), &whiteBrush);
        y += 25;

        // Current coordinates
        if (currentCoords.x != 0 || currentCoords.z != 0) {
            std::wstringstream coordText;
            coordText << L"Player: (" << currentCoords.x << L", " << currentCoords.z << L")";
            graphics.DrawString(coordText.str().c_str(), -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 20;
        }
        else {
            graphics.DrawString(L"Press hotkey to find treasures", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 20;
        }

        // Show treasure results
        if (!nearestTreasures.empty()) {
            graphics.DrawString(L"Nearest Buried Treasures:", -1, &infoFont,
                PointF((REAL)x, (REAL)y), &greenBrush);
            y += 22;

            // Header
            graphics.DrawString(L"Rank  X Coord   Z Coord   Distance", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 18;

            // Show up to 6 treasures in overlay
            int maxShow = std::min(6, (int)nearestTreasures.size());
            for (int i = 0; i < maxShow; i++) {
                const auto& treasure = nearestTreasures[i];

                SolidBrush* brush = (i == 0) ? &greenBrush :
                    (i < 3) ? &yellowBrush : &lightGrayBrush;

                std::wstringstream treasureText;
                treasureText << L"#" << (i + 1) << L"   "
                    << std::setw(6) << treasure.x << L"   "
                    << std::setw(6) << treasure.z << L"   "
                    << std::fixed << std::setprecision(1)
                    << treasure.distance << L"m";

                graphics.DrawString(treasureText.str().c_str(), -1, &coordFont,
                    PointF((REAL)x, (REAL)y), brush);
                y += 20;
            }

            y += 10;
            graphics.DrawString(L"Drag to move • Press hotkey to refresh", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
        }

        EndPaint(hWnd, &ps);
    }
    break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Copy results to clipboard
void CopyTreasuresToClipboard() {
    if (nearestTreasures.empty()) return;

    std::wstringstream ss;
    ss << L"Nearest Buried Treasures:\n";
    ss << L"Player Position: (" << currentCoords.x << L", " << currentCoords.z << L")\n\n";

    int count = std::min(5, (int)nearestTreasures.size());
    for (int i = 0; i < count; i++) {
        const auto& treasure = nearestTreasures[i];
        ss << L"#" << (i + 1) << L": (" << treasure.x << L", " << treasure.z
            << L") - Distance: " << std::fixed << std::setprecision(1)
            << treasure.distance << L" blocks\n";
    }

    std::wstring text = ss.str();

    OpenClipboard(NULL);
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (hGlob) {
        memcpy(GlobalLock(hGlob), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(hGlob);
        SetClipboardData(CF_UNICODETEXT, hGlob);
    }
    CloseClipboard();
}

// Get key name for display
std::wstring GetKeyName(int vkCode) {
    switch (vkCode) {
    case VK_F1: return L"F1";
    case VK_F2: return L"F2";
    case VK_F3: return L"F3";
    case VK_F4: return L"F4";
    case VK_F5: return L"F5";
    case VK_F6: return L"F6";
    case VK_F7: return L"F7";
    case VK_F8: return L"F8";
    case VK_F9: return L"F9";
    case VK_F10: return L"F10";
    case VK_F11: return L"F11";
    case VK_F12: return L"F12";
    case VK_TAB: return L"TAB";
    case VK_SPACE: return L"SPACE";
    case VK_RETURN: return L"ENTER";
    case VK_ESCAPE: return L"ESC";
    case VK_INSERT: return L"INSERT";
    case VK_DELETE: return L"DELETE";
    case VK_HOME: return L"HOME";
    case VK_END: return L"END";
    case VK_PRIOR: return L"PAGE UP";
    case VK_NEXT: return L"PAGE DOWN";
    case VK_UP: return L"UP ARROW";
    case VK_DOWN: return L"DOWN ARROW";
    case VK_LEFT: return L"LEFT ARROW";
    case VK_RIGHT: return L"RIGHT ARROW";
    default:
        if (vkCode >= 'A' && vkCode <= 'Z') {
            return std::wstring(1, (wchar_t)vkCode);
        }
        if (vkCode >= '0' && vkCode <= '9') {
            return std::wstring(1, (wchar_t)vkCode);
        }
        return L"KEY_" + std::to_wstring(vkCode);
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == 1001) { // Change treasure hotkey button
            waitingForHotkey = true;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (LOWORD(wParam) == 1002) { // Change overlay hotkey button
            waitingForOverlayHotkey = true;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_HOTKEY:
        if (wParam == 1) { // Treasure finder hotkey
            HWND mcHwnd = FindWindow(NULL, L"Minecraft");
            if (!mcHwnd) {
                // Try alternative window names for Minecraft Bedrock
                mcHwnd = FindWindow(NULL, L"Minecraft for Windows 10");
            }

            if (mcHwnd && GetShownCoordinates(mcHwnd, &currentCoords)) {
                FindNearestTreasures(currentCoords.x, currentCoords.z);
                CopyTreasuresToClipboard();

                // Show overlay when treasures are found
                if (!nearestTreasures.empty()) {
                    ShowOverlay();
                }
                else {
                    HideOverlay();
                }

                UpdateOverlay();
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        else if (wParam == 2) { // Overlay toggle hotkey
            ToggleOverlay();
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.Clear(Color(25, 25, 35));

        FontFamily fontFamily(L"Segoe UI");
        Font headerFont(&fontFamily, 16, FontStyleBold, UnitPixel);
        Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
        Font smallFont(&fontFamily, 10, FontStyleRegular, UnitPixel);

        SolidBrush whiteBrush(Color(240, 240, 240));
        SolidBrush grayBrush(Color(180, 180, 180));
        SolidBrush greenBrush(Color(120, 220, 120));
        SolidBrush yellowBrush(Color(255, 220, 100));
        SolidBrush cyanBrush(Color(100, 200, 255));
        SolidBrush redBrush(Color(255, 100, 100));

        int marginX = 20;
        int y = 20;

        // Header
        graphics.DrawString(L"Minecraft Buried Treasure Finder", -1, &headerFont,
            PointF((REAL)marginX, (REAL)y), &cyanBrush);
        y += 40;

        // Hotkey configuration
        std::wstringstream hotkeyText;
        hotkeyText << L"Find Treasure Hotkey: " << GetKeyName(currentHotkey);
        if (waitingForHotkey) {
            hotkeyText << L" (Press new key...)";
        }
        SolidBrush* hotkeyBrush = waitingForHotkey ? &yellowBrush : &grayBrush;
        graphics.DrawString(hotkeyText.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), hotkeyBrush);
        y += 30;

        // Overlay toggle hotkey configuration
        std::wstringstream overlayHotkeyText;
        overlayHotkeyText << L"Toggle Overlay Hotkey: " << GetKeyName(overlayToggleHotkey);
        if (waitingForOverlayHotkey) {
            overlayHotkeyText << L" (Press new key...)";
        }
        SolidBrush* overlayHotkeyBrush = waitingForOverlayHotkey ? &yellowBrush : &grayBrush;
        graphics.DrawString(overlayHotkeyText.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), overlayHotkeyBrush);
        y += 35;

        // Overlay status
        std::wstringstream overlayStatus;
        overlayStatus << L"Overlay Status: " << (overlayVisible ? L"Visible" : L"Hidden");
        graphics.DrawString(overlayStatus.str().c_str(), -1, &smallFont,
            PointF((REAL)marginX, (REAL)y), overlayVisible ? &greenBrush : &redBrush);
        y += 25;

        // Current coordinates
        std::wstringstream coordText;
        coordText << L"Current Position:\n"
            << L"X: " << currentCoords.x << L"\n"
            << L"Y: " << currentCoords.y << L"\n"
            << L"Z: " << currentCoords.z;
        graphics.DrawString(coordText.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), &grayBrush);
        y += 85;

        // Instructions or results
        if (nearestTreasures.empty()) {
            std::wstringstream instrText;
            instrText << L"Instructions:\n"
                << L"1. Press " << GetKeyName(currentHotkey) << L" to find nearest buried treasures\n"
                << L"2. Press " << GetKeyName(overlayToggleHotkey) << L" to toggle overlay visibility\n"
                << L"3. Results will be copied to clipboard automatically\n"
                << L"4. Overlay shows treasure locations (drag to move)";
            graphics.DrawString(instrText.str().c_str(), -1, &font,
                PointF((REAL)marginX, (REAL)y), &whiteBrush);
        }
        else {
            graphics.DrawString(L"Nearest Buried Treasures:", -1, &font,
                PointF((REAL)marginX, (REAL)y), &greenBrush);
            y += 25;

            // Show overlay status info
            std::wstringstream overlayInfo;
            overlayInfo << L"Overlay: " << (overlayVisible ? L"Active (drag to move)" : L"Hidden");
            graphics.DrawString(overlayInfo.str().c_str(), -1, &smallFont,
                PointF((REAL)marginX, (REAL)y), overlayVisible ? &cyanBrush : &redBrush);
            y += 20;

            // Table header
            graphics.DrawString(L"Rank  X Coord   Z Coord   Distance", -1, &smallFont,
                PointF((REAL)marginX, (REAL)y), &grayBrush);
            y += 20;

            // Show treasure locations
            int maxShow = std::min(8, (int)nearestTreasures.size());
            for (int i = 0; i < maxShow; i++) {
                const auto& treasure = nearestTreasures[i];

                std::wstringstream treasureText;
                treasureText << L"#" << (i + 1) << L"    "
                    << std::setw(6) << treasure.x << L"    "
                    << std::setw(6) << treasure.z << L"    "
                    << std::fixed << std::setprecision(1)
                    << treasure.distance << L" blocks";

                SolidBrush* brush = (i == 0) ? &greenBrush :
                    (i < 3) ? &yellowBrush : &grayBrush;

                graphics.DrawString(treasureText.str().c_str(), -1, &font,
                    PointF((REAL)marginX, (REAL)y), brush);
                y += 20;
            }

            y += 10;
            std::wstringstream copyText;
            copyText << L"Top 5 locations copied to clipboard. Press "
                << GetKeyName(currentHotkey) << L" to refresh.";
            graphics.DrawString(copyText.str().c_str(), -1, &smallFont,
                PointF((REAL)marginX, (REAL)y), &whiteBrush);
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_CREATE:
    {
        // Store main window handle globally
        g_hMainWnd = hWnd;

        // Install low-level keyboard hook for hotkey changing
        g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

        // Load settings first
        LoadSettings();

        // Register overlay window class
        MyRegisterOverlayClass(GetModuleHandle(NULL));

        // Initialize overlay
        InitOverlay(GetModuleHandle(NULL));

        // Create change hotkey buttons
        CreateWindow(L"BUTTON", L"Change Find Hotkey", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            250, 55, 140, 25, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);

        CreateWindow(L"BUTTON", L"Change Toggle Hotkey", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            250, 85, 140, 25, hWnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

        // Register hotkeys
        RegisterHotKey(hWnd, 1, 0, currentHotkey);
        RegisterHotKey(hWnd, 2, 0, overlayToggleHotkey);
    }
    break;

    case WM_DESTROY:
        // Save settings before closing
        SaveSettings();

        // Unhook keyboard hook
        if (g_hKeyboardHook) {
            UnhookWindowsHookEx(g_hKeyboardHook);
        }

        // Unregister hotkeys
        UnregisterHotKey(hWnd, 1);
        UnregisterHotKey(hWnd, 2);

        // Destroy overlay window
        if (hOverlayWnd) {
            DestroyWindow(hOverlayWnd);
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register window class
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"TreasureFinderWindow";
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wcex);

    // Create window
    HWND hWnd = CreateWindow(L"TreasureFinderWindow", L"Minecraft Buried Treasure Finder",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 500, 650, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
