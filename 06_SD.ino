/*******************************************************************************
 *  File   : SD.ino
 *  Author : Hideki Sakakibara @ HVD
 *  Update : 2025/07/10
 *  Bord   : M5Stack CoreS3 SE
 *  Lib    : -
 *  Overview : 
 *******************************************************************************/

/*******************************************************************************
 * User defined constant/variables definition
 *******************************************************************************/

/* ------------------------ *
 *   Macro definition
 * ------------------------ */

  /* --- ピン番号 ------------------------------------ */
  #define SPI_CS_PIN   4
  // # define SPI_MOSI_PIN 7 // G37
  // # define SPI_MISO_PIN 9 // G35
  // # define SPI_SCK_PIN 11 // G36

  /* --- 数字判別 ------------------------------------ */
  #define CHARACTER                   0     // 文字

  /* --- 極性判定 ------------------------------------ */
  #define SUCCESS                     1     // 成功
  #define FAILED                      0     // 失敗

  /* --- 極性判定 ------------------------------------ */
  #define NORMAL                      1     // 正常
  #define ABNORMAL                    0     // 異常


/* ------------------------ *
 *   Constant
 * ------------------------ */


/* ------------------------ *
 *   Global variables
 * ------------------------ */
  U1 gf_sd_init;                   // SDカードの初期化状態
  U1 gf_sd_open;                   // SDカードの開閉状態
  U1 gf_load_state[CSV_FILE_NUM];  // CSV読取り状態判定
  U1 gf_load_finish;               // CSV読取り状態

  S4 s4_csv_row_max;               // 行数の最大
  U1 f_csv_row_end;                // csv最終行到達フラグ

  File csv_file;


/*******************************************************************************
 * User defined functions
 *******************************************************************************

/* ------------------------------------------------------  *
 *   csvファイルオープン
 * ------------------------------------------------------- */
void open_csv(U1 u1_load_file_num)
{
  /* --- SDカードの初期化確認 ------------------------ */
  if (SD.begin(SPI_CS_PIN) == ABNORMAL) {
      gf_sd_init = FAILED;
      Serial.println("SDcard initialized error");
  } else {
      gf_sd_init = SUCCESS;
      Serial.println("SDcard initialized success");
  }

  /* --- SDカードの初期化が成功した場合、ファイルオープン処理 ----------*/
  if (gf_sd_init == SUCCESS) {
      /* --- csvファイルオープン -------------------------*/
      switch (u1_load_file_num) {
          case 0:   csv_file = SD.open(CSV_FILE_00);  break;
          // case 1:   csv_file = SD.open(CSV_FILE_01);  break;
          // case 2:   csv_file = SD.open(CSV_FILE_02);  break;
          default:  gf_sd_init = FAILED;
      }
  }

  if (gf_sd_init == SUCCESS) {
      /* --- csvファイルが開けたかの確認 ------------------*/
      if (csv_file == ABNORMAL) {
          gf_sd_open = FAILED;
          Serial.println("CSV File error");
      } else {
          gf_sd_open = SUCCESS;
          Serial.println("CSV File opened");
      }
  }
}


/* ------------------------------------------------------  *
 *   全てのcsvファイルを読み込めたか判断
 *  ------------------------------------------------------ */
void judge_csv()
{
    gf_load_finish = SUCCESS;  // 初期化

    /* 全ファイルが読込みできたか判定 */ 
    for (int i=0; i<CSV_FILE_NUM; i++) {
        if (gf_load_state[i] == FAILED) {
            gf_load_finish = FAILED;
        }
    }

    /* デバッグ用 */
    if (gf_load_finish == SUCCESS) {   // 読込み成功
        Serial.println("csv load finish");
    } else {                          // 読込み失敗
        Serial.println("csv load error");
    }
}


/* ------------------------------------------------------  *
 *   時間列:実数から整数に変換
 * ------------------------------------------------------- */
S4 conv_2_time(float val_csv)
{
    S4 s4_val_calc;

    s4_val_calc = (S4)(val_csv * 1000);
    
    return s4_val_calc;
}


/* ------------------------------------------------------  *
 *   呼吸音列:実数から整数に変換
 * ------------------------------------------------------- */
S4 conv_2_breathing(float val_csv)
{
    float val_lim;
    S4 s4_val_calc;

    /* リミッター処理 */
    if (val_csv < INPUT_MIN_BREATH) {        // 最小
        val_lim = INPUT_MIN_BREATH;
    } else if (val_csv > INPUT_MAX_BREATH) { // 最大
        val_lim = INPUT_MAX_BREATH;
    } else {
        val_lim = val_csv;
    }

    s4_val_calc = (S4)(val_lim * (OUTPUT_MAX_BREATH - OUTPUT_MIN_BREATH) / (INPUT_MAX_BREATH - INPUT_MIN_BREATH));

    return s4_val_calc;
}


/* -----------------------------------------------------  *
 *   心音列:実数から整数に変換
 * ------------------------------------------------------- */
S4 conv_2_heartbeat(float val_csv)
{
    float val_lim;
    S4 s4_val_calc;

    /* リミッター処理 */
    if (val_csv < INPUT_MIN_HEART) {        // 最小
        val_lim = INPUT_MIN_HEART;
    } else if (val_csv > INPUT_MAX_HEART) { // 最大
        val_lim = INPUT_MAX_HEART;
    } else {
        val_lim = val_csv;
    }

    s4_val_calc = (S4)(val_lim * (OUTPUT_MAX_HEART - OUTPUT_MIN_HEART) / (INPUT_MAX_HEART - INPUT_MIN_HEART));

    return s4_val_calc;
}


/* ------------------------ *
 *   HeartBeat -> Motor ON 時間に変換
 * ------------------------ */
void get_time_MTON(S4 ts4_time, S4 ts4_HB)
{
    static S4 ts4_time_last[8] = {0};
    static S4 ts4_HB_last[8] = {0};
    static U2 tu2_index = 0;

    ts4_time_last[7] = ts4_time_last[6];  // ↑ 古い
    ts4_time_last[6] = ts4_time_last[5];
    ts4_time_last[5] = ts4_time_last[4];
    ts4_time_last[4] = ts4_time_last[3];
    ts4_time_last[3] = ts4_time_last[2];
    ts4_time_last[2] = ts4_time_last[1];
    ts4_time_last[1] = ts4_time_last[0];
    ts4_time_last[0] = ts4_time;          // ↓ 新しい

    ts4_HB_last[7]   = ts4_HB_last[6];    // ↑ 古い
    ts4_HB_last[6]   = ts4_HB_last[5];
    ts4_HB_last[5]   = ts4_HB_last[4];
    ts4_HB_last[4]   = ts4_HB_last[3];
    ts4_HB_last[3]   = ts4_HB_last[2];
    ts4_HB_last[2]   = ts4_HB_last[1];
    ts4_HB_last[1]   = ts4_HB_last[0];
    ts4_HB_last[0]   = ts4_HB;            // ↓ 新しい

    // if (   (ts4_HB_last[7] <  0)
    //     && (ts4_HB_last[6] <  0)
    //     && (ts4_HB_last[5] <  0)
    //     && (ts4_HB_last[4] <  0)
    //     && (ts4_HB_last[3] >= 0) && (ts4_time_last[3] > 0)
    //     && (ts4_HB_last[2] >  0)
    //     && (ts4_HB_last[1] >  0)
    //     && (ts4_HB_last[0] >  0) ) {

    /*
     *  心音の谷を検出
     */
    // if (   (ts4_HB_last[7] < 0)
    //     && (ts4_HB_last[6] < 0)
    //     && (ts4_HB_last[5] < 0)
    //     && (ts4_HB_last[4] < 0)
    //     && (ts4_HB_last[3] < 0)
    //     && (ts4_HB_last[2] < 0)
    //     && (ts4_HB_last[1] < 0)
    //     && (ts4_HB_last[0] < 0)
    //     && (ts4_HB_last[7] >= ts4_HB_last[3])
    //     && (ts4_HB_last[6] >= ts4_HB_last[3])
    //     && (ts4_HB_last[5] >= ts4_HB_last[3])
    //     && (ts4_HB_last[4] >= ts4_HB_last[3]) && (ts4_time_last[3] > 0)
    //     && (ts4_HB_last[3] <  ts4_HB_last[2])
    //     && (ts4_HB_last[3] <  ts4_HB_last[1])
    //     && (ts4_HB_last[3] <  ts4_HB_last[0]) ) {

    /*
     *  心音の山を検出
     */
    if (   (ts4_HB_last[7] > 0)
        && (ts4_HB_last[6] > 0)
        && (ts4_HB_last[5] > 0)
        && (ts4_HB_last[4] > 0)
        && (ts4_HB_last[3] > 0)
        && (ts4_HB_last[2] > 0)
        && (ts4_HB_last[1] > 0)
        && (ts4_HB_last[0] > 0)
        && (ts4_HB_last[7] <= ts4_HB_last[3])
        && (ts4_HB_last[6] <= ts4_HB_last[3])
        && (ts4_HB_last[5] <= ts4_HB_last[3])
        && (ts4_HB_last[4] <= ts4_HB_last[3]) && (ts4_time_last[3] > 0)
        && (ts4_HB_last[3] >  ts4_HB_last[2])
        && (ts4_HB_last[3] >  ts4_HB_last[1])
        && (ts4_HB_last[3] >  ts4_HB_last[0]) ) {

        u4_time_mt_on[tu2_index] = ts4_time_last[3];
        tu2_index++;
        u2_time_index = tu2_index;
    }

}



/* ------------------------------------------------------  *
 *   csvファイルの中身を配列に格納
 * ------------------------------------------------------- */
void read_csv(U1 u1_load_file_num)
{
  U1 f_judge_convert;             // 文字列数字変換が可能か判断
  U1 u1_buf_len;                  // 一行の要素数
  float val_csv;                  // 文字列から変換した実数
  U1 u1_num_start;                // 実数変換前の文字列の開始地点
  U2 u2_row           = 0;        // 格納する配列の行番号
  U1 u1_col           = 0;        // 格納する配列の列番号

  /* csvファイルにまだ読み取り可能な文字(byte)がある場合 */
  while (csv_file.available() > 0){
      f_judge_convert  = ENABLE;

      /* 一行を抽出 */
      String buf = csv_file.readStringUntil('\n');

      /* 1文字目が数字ではない場合、文字列数字変換が可能 */
      if (isDigit(buf[0]) == CHARACTER) {
          f_judge_convert = DISABLE;
      }
      

      /* 文字列数字変換が可能な場合、変換処理 */
      if (f_judge_convert == ENABLE) {
          /* 文字列の要素数の取得 */
          u1_buf_len = buf.length();

          u1_num_start = 0; // カンマの分割場所のリセット
          u1_col = 0;       // 格納する配列の列数リセット

          /* --- 文字列をカンマで分割し、それぞれの文字列を実数へ変換 ----------------------------- */
          for (int i=0; i<u1_buf_len; i++) {
              /* カンマか行末の場合、実数に変換 */
              if ((buf[i] == ',') || (i == (u1_buf_len - 1))) {
                  val_csv = (buf.substring(u1_num_start, i-1)).toFloat();

                  /* 実数から整数に変換 */
                  switch (u1_col) {
                      case EN_TIME:
                          s4_vital[u1_load_file_num][u2_row][u1_col] = conv_2_time(val_csv);
                          break;
                      case EN_BREATHING:
                          s4_vital[u1_load_file_num][u2_row][u1_col] = conv_2_breathing(val_csv);
                          break;
                      case EN_HEARTBEAT:
                          s4_vital[u1_load_file_num][u2_row][u1_col] = conv_2_heartbeat(val_csv);

                          get_time_MTON( s4_vital[u1_load_file_num][u2_row][EN_TIME],
                                         s4_vital[u1_load_file_num][u2_row][EN_HEARTBEAT] );

                          break;
                      default:
                          // 上記以外の場合は配列vitalに格納しない
                          break;
                  }

                  u1_col++;
                  u1_num_start = i + 1;
              }
          }

          u2_row++; // U2_CSV_ROWが上限
      }
  }
}


/* ------------------------ *
 *   csv->配列に変換
 * ------------------------ */
void make_csv_table()
{
    /* csvファイルの読み込み状況を初期化 */
    for (int i=0; i<CSV_FILE_NUM; i++){
        gf_load_state[i] = FAILED;
    }

    /*　定義されたファイル数の読込みが終わっていない場合 */
    for (int i=0; i<CSV_FILE_NUM; i++) {
        gf_sd_init = FAILED;
        gf_sd_open = FAILED;

        /* csvファイルを開く */
        open_csv(i);

        /* SDカードの初期化かオープンが出来ていない場合、処理をスキップ */
        if (   (gf_sd_init == SUCCESS)
            && (gf_sd_open == SUCCESS)) {
            /* csvファイルを読込み、配列に格納 */
            read_csv(i);

            /* csvファイルが読込めたか判定 */
            if (csv_file.available() == 0) {
                gf_load_state[i] = SUCCESS;
            }

            /* csvファイルを閉じる */
            csv_file.close();
        }
    }

    /* 全ファイルが読込みできたか判定*/
    judge_csv();

    /* デバッグ用シリアルモニタ表示 */
    if (DEBUG_CSV_TABLE) {
        for (int i=0; i<CSV_FILE_NUM; i++) {

            for (int j=0; j<U2_CSV_ROW; j++) {
                Serial.print(j);
                Serial.print(" ");

                for (int k=0; k<U1_CSV_COL; k++) {

                    if (   (k == EN_TIME)
                        && (s4_vital[i][j][EN_TIME] == 0)) {

                        s4_csv_row_max = (S4)j;
                        f_csv_row_end = ON;
                        break;
                    }
                    else {
                        Serial.print(s4_vital[i][j][k]);
                        Serial.print(" ");
                    }
                }
                Serial.println("");
                if (f_csv_row_end == ON) {
                    break;
                }
            }
            Serial.println("");
        }
    }

    /* デバッグ用シリアルモニタ表示 */
    if (DEBUG_TIME_MTON) {
        for (int i=0; i<u2_time_index; i++) {
              Serial.print(i);
              Serial.print(", ");
              Serial.print(u4_time_mt_on[i]);   
              Serial.println();
        }
        Serial.print(" time index: ");
        Serial.print(u2_time_index);           
        Serial.println();
    }
}
