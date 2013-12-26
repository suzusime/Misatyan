#include <iostream>
#include <iomanip>
#include "stdio.h"
#include "string.h"
using namespace std;

typedef struct {
     unsigned char r;    // r(赤) の諧調  0 ～ 15
     unsigned char g;    // g(緑) の諧調  0 ～ 15
     unsigned char b;    // b(青) の諧調  0 ～ 15
} PAL_T;        //        上位 4bit は 必ず 0

PAL_T Palette[16];      // Palette[0]  color 0 のパレット
                        //         |         |
                        //        [15]       15

typedef struct {
     short int CellNo;    // 何番目のセルか？
     short int TimeLen;   // 表示時間（1/60 単位）
} Senario;

typedef struct {         // PCK / P16 フォーマットのヘッダ
	short int _x;              //       Ｘ方向の幅（bit 単位）
	short int _y;              //       Ｙ方向の幅（line 単位）
} PckHead;


typedef struct {
    char header[8];     // $_MISA_$ という文字列         $_MISA_$ （デジタル）
                        // または %_MISA_% という文字列  %_MISA_% （アナログ）
    short int EndCode1;       // 改行コード                    0x0a0d
    short int EndCode2;       // ファイル終了コード と 0       0x001a
    short int _ID_;           // 未使用。             0xf06c
    short int X_Width;        // 横方向の幅。byte 単位。
    short int Y_Width;        // 縦方向の幅。line 単位。
    short int Def_X;          // デフォルト表示Ｘ座標。
    short int Def_Y;          // デフォルト表示Ｙ座標。
    short int MaxAnimeCells;  // セルの枚数。
    short int CellSize;       // 一枚のセル（.PCK / .P16）の大きさ。
    //char *er1;          // メッセージ１へのポインタ。
    //char *er2;          // メッセージ２へのポインタ。
    //char *er3;          // メッセージ３へのポインタ。
    //char *er4;          // 未使用。
    short int er1;          // メッセージ１へのポインタ。
    short int er2;          // メッセージ２へのポインタ。
    short int er3;          // メッセージ３へのポインタ。
    short int er4;          // 未使用。

    //PAL_T *rgb;         // FORMAT %_MISA_% の場合、RGB 領域へのポインタ。
                        // FORMAT $_MISA_$ の場合、未使用。
    short int rgb;         // FORMAT %_MISA_% の場合、RGB 領域へのポインタ。
                        // FORMAT $_MISA_$ の場合、未使用。
    //Senario *_Conte;    // コンテ領域の先頭へのポインタ。
    short int  _Conte;    // コンテ領域の先頭へのポインタ。
    //void *AnimeCellData;// セル領域の先頭へのポインタ。
    short int AnimeCellData;// セル領域の先頭へのポインタ。
    char reserve[8];    // 予約領域。未使用。
} MisaHead;

class MisaData{
public:
	char er1[256];
	char er2[256];
	char er3[256];
	short int conte_size;
	Senario* sen;
	PckHead* pck_head;
	unsigned char** pck_data;

	MisaData(){};
	~MisaData(){};
};

int road_rgb( FILE *fp, short int rgb_offset )
{
	fseek( fp, rgb_offset, SEEK_SET );
	fread( Palette, sizeof(PAL_T), 16, fp );
	return 0;
}

short int road_conte_size( FILE *fp, short int maxcells, short int conte_offset )//コンテ枚数取得
{
	fseek( fp, conte_offset, SEEK_SET );
	short int conte_size = 0;
	short int check_no = 0;
	Senario tmp_sen;
	for ( short int i = 0; check_no != -1; i++ ){
		fread( &tmp_sen, sizeof(Senario), 1,fp );
		check_no = tmp_sen.CellNo;
		conte_size++;
	}
	return conte_size;
}

int road_conte( FILE *fp, Senario *senpoint, short int conte_size, short int conte_offset )//コンテ取得
{
	fseek( fp, conte_offset, SEEK_SET );
	fread( senpoint, sizeof(Senario), conte_size,fp );
	return 0;
}

int road_cell( FILE *fp, PckHead *pckheader, unsigned char **pckdata, short int maxcells, short int cellsize, short int cell_offset )//P16データ取得
{
	for ( short int i = 0; i < maxcells; i++ ){
		fseek( fp, cell_offset + i * cellsize, SEEK_SET );
		fread( pckheader + i, sizeof(PckHead), 1,fp );
		fread( *( pckdata + i ), sizeof(cellsize) - sizeof(PckHead), 1,fp );
	}
	return 0;
}



int main(){
	//前書き
	cout << "//----------------------------------------------------------\n";
	cout << "// 【未沙ちゃんファイル変換プログラム】\n";
	cout << "//\n";
	cout << "//                                                  試作零号\n";
	cout << "//\n";
	cout << "// （注意）\n";
	cout << "//    使用に因って生じた一切に対し制作者は責任を負わない。\n";
	cout << "//----------------------------------------------------------\n" <<endl;
	//前書き以上
	
	cout << "<<変換処理進捗状況>>" <<endl;
	FILE *fp;	/* (1)ファイルポインタの宣言 */
	fpos_t fpos, tmppos; //ファイル位置
	if ( (fp = fopen( "D:\\test\\test.msa", "rb" )) == NULL ){
		cout << "警告：ファイルを開けませんでした。" <<endl;
		exit(EXIT_FAILURE);	/* (3)エラーの場合は通常、異常終了する */
	}

	MisaHead Head1;
	//ファイル読込部
	fgetpos(fp, &fpos);//ファイル位置の退避
	fread( Head1.header, sizeof(Head1.header), 1, fp );

	////ファイル確認部
	if ( strncmp ( Head1.header, "%_MISA_%", 8 ) != 0 ){
		cout << "警告：このファイルはアナログ16色未沙ちゃんデータではありません。" <<endl;
		exit(EXIT_FAILURE);
	}

	////ヘッダ読込部
	fread( &Head1.EndCode1, sizeof(short int), 1, fp );
	fread( &Head1.EndCode2, sizeof(short int), 1, fp );
	fread( &Head1._ID_, sizeof(short int), 1, fp );
	fread( &Head1.X_Width, sizeof(short int), 1, fp );
	fread( &Head1.Y_Width, sizeof(short int), 1, fp );
	fread( &Head1.Def_X, sizeof(short int), 1,fp );
	fread( &Head1.Def_Y, sizeof(short int), 1,fp );
	fread( &Head1.MaxAnimeCells, sizeof(short int), 1,fp );
	fread( &Head1.CellSize, sizeof(short int), 1,fp );
	fread( &Head1.er1, sizeof(short int), 1,fp );
	fread( &Head1.er2, sizeof(short int), 1,fp );
	fread( &Head1.er3, sizeof(short int), 1,fp );
	fread( &Head1.er4, sizeof(short int), 1,fp );
	fread( &Head1.rgb, sizeof(short int), 1,fp );
	fread( &Head1._Conte, sizeof(short int), 1,fp );
	fread( &Head1.AnimeCellData, sizeof(short int), 1,fp );
	fread( Head1.reserve, sizeof(Head1.reserve), 1,fp );
	////データ取得部
	MisaData Data1;
	fseek( fp, Head1.er1, SEEK_SET );
	fgets( Data1.er1, 256, fp );
	fseek( fp, Head1.er2, SEEK_SET );
	fgets( Data1.er2, 256, fp );
	fseek( fp, Head1.er3, SEEK_SET );
	fgets( Data1.er3, 256, fp );
	road_rgb( fp, Head1.rgb );
	Data1.conte_size = road_conte_size( fp, Head1.MaxAnimeCells, Head1._Conte );//コンテ枚数取得
	cout << "シナリオ用メモリ確保中..." <<endl;
	Data1.sen = new Senario[ Data1.conte_size ];//シナリオ用メモリ確保
	cout << "シナリオ用メモリ確保完了。" <<endl;
	cout << "PCKヘッダ用メモリ確保中..." <<endl;
	Data1.pck_head = new PckHead[ Head1.MaxAnimeCells ];//PCKヘッダ用メモリ確保
	cout << "PCKヘッダ用メモリ確保完了。" <<endl;
	cout << "PCKデータ用メモリ確保中..." <<endl;
	Data1.pck_data = new unsigned char*[ Head1.MaxAnimeCells ];
	for ( short int i = 0; i < Head1.MaxAnimeCells; i++ ){
		Data1.pck_data[i] = new unsigned char[ Head1.CellSize ];
	}//PCKデータ用メモリ確保
	cout << "PCKデータ用メモリ確保完了。" <<endl;
	road_conte( fp, Data1.sen, Data1.conte_size, Head1._Conte );
	road_cell( fp, Data1.pck_head, Data1.pck_data, Head1.MaxAnimeCells, Head1.CellSize, Head1.AnimeCellData);
	
	fclose(fp);	/* (5)ファイルのクローズ */
	
	//表示部
	cout << "\n<<ファイル情報>>" <<endl;
	/*cout << Head1.header <<endl;*/
/*
	cout << Head1.EndCode1 <<endl;
	cout << Head1.EndCode2 <<endl;
	cout << Head1._ID_ <<endl;
*/	
	cout << "横    幅：" << Head1.X_Width <<endl;
	cout << "縦    幅：" << Head1.Y_Width <<endl;
	cout << "横 位 置：" << Head1.Def_X <<endl;
	cout << "縦 位 置：" << Head1.Def_Y <<endl;
	cout << "セル枚数：" << Head1.MaxAnimeCells <<endl;
	cout << "一枚容量：" << Head1.CellSize <<endl;
/*	cout << Head1.reserve <<endl; */
	cout << "伝達文１：" << Data1.er1 << endl;
	cout << "伝達文２：" << Data1.er2 << endl;
	cout << "伝達文３：" << Data1.er3 << endl;
	cout << "コンテ数：" << Data1.conte_size <<endl;

	cout << "パレット：" <<endl;
	for( short int i = 0; i < 16; i++ ){
		cout << " " << setw(2) << setfill(' ') << i << "番" << " [" << setw(2) << setfill(' ') << (short int)Palette[i].r << "] [" << setw(2) << setfill(' ') << (short int)Palette[i].g << "] [" << setw(2) << setfill(' ') << (short int)Palette[i].b << "]" <<endl;
	}

	cout << "コ ン テ：" <<endl;
	for( short int i = 0; i < Data1.conte_size; i++){
		cout  << " " << setw(3) << setfill(' ') << i << " " << setw(3) << setfill(' ') << Data1.sen[i].CellNo << "番の絵を" << setw(5) << setfill(' ') << Data1.sen[i].TimeLen << "フレーム" <<endl;
	}

	cout << "セル幅高：" <<endl;
	for( short int i = 0; i < Head1.MaxAnimeCells; i++){
		cout  << " " << setw(3) << setfill(' ') << i << " " << setw(3) << setfill(' ') << Data1.pck_head[i]._x << ", " << setw(3) << setfill(' ') << Data1.pck_head[i]._y <<endl;
	}

	cout << "\n<<終了処理進捗状況>>" <<endl;

	//メモリ解放
	cout << "シナリオ用メモリ解放中..." <<endl;
	delete[] Data1.sen;
	cout << "シナリオ用メモリ解放完了。" <<endl;
	cout << "PCKヘッダ用メモリ解放中..." <<endl;
	delete[] Data1.pck_head;
	cout << "PCKヘッダ用メモリ解放完了。" <<endl;
	cout << "PCKデータ用メモリ解放中..." <<endl;
	for ( short int i = 0; i < Head1.MaxAnimeCells; i++ ){
		delete[] Data1.pck_data[i];
	}
	delete[] Data1.pck_data;
	cout << "PCKデータ用メモリ解放完了。" <<endl;

	//一時停止
	int aiu;
	cin >> aiu;

	return 0;
}

