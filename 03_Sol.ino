/*******************************************************************************
 *  File   : Sol.ino
 *  Author : Suguru Aoki @ HVD
 *  Update : 2025/04/17
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
  // M5stack coreS3 : DOUT
  #define P_SOL1      5         // G5 : GPIO
  #define P_SOL2      6         // G6 : GPIO
  #define P_SOL3      7         // G7 : GPIO
  // #define P_SOL4  10         // G10: ADC (予備)

  #define OPEN        1         // 電磁弁：開
  #define CLOSE       0         // 電磁弁：閉


/* ------------------------ *
 *   Constant
 * ------------------------ */
  enum EN_SOL_NO {  SOL_NO1 = 0,        // 0: SOL 1
                    SOL_NO2,            // 1: SOL 2
                    SOL_NO3, };         // 2: SOL 3

  enum EN_SOL_SET { EN_SOL_ON = 0,      // 0: Sol ON/OFF
                    EN_SOL_PIN  };      // 1: Sol Port No(Gxx)

  const U1 U1_SOL_NUM = SOL_NO3 + 1;    // ブラダ数

  const U2 U2_CNT_SOL_OFF = (U2)(TIME_SOL_OFF / MAIN_CYCLE);

/* ------------------------ *
 *   Global variables
 * ------------------------ */
                                                          // Use ,  Port No
  volatile U1 u1_sol_seting[U1_SOL_NUM][EN_SOL_PIN + 1] = { { ON ,  P_SOL1 },      // SOL 1 設定
                                                            { ON ,  P_SOL2 },      // SOL 2 設定
                                                            { ON ,  P_SOL3 } };    // SOL 3 設定

  volatile U1 f_sol_req;                   // Solenoid drive request
  volatile U2 u2_cnt_sol_off[U1_SOL_NUM];  // Solenoid drive time cnt



/*******************************************************************************
 * User defined functions
 *******************************************************************************/

/* ------------------------ *
 *   GPIO出力ポート初期化
 * ------------------------ */
 void init_Sol(void)
 {

    Serial.println("Solenoid init start...");

    for (int i = 0; i < U1_SOL_NUM; i++) {
        pinMode(u1_sol_seting[i][EN_SOL_PIN], OUTPUT);
        digitalWrite(u1_sol_seting[i][EN_SOL_PIN], OFF);
    }

    Serial.println("Solenoid init finish !");
    Serial.println("");

    f_sol_req = OFF;
    for (int i = 0; i < U1_SOL_NUM; i++) {
        u2_cnt_sol_off[i] = 0;
    }
 }

/* ------------------------ *
 *   ソレノイドバルブ駆動要求判定
 * ------------------------ */
void judge_Sol(void)
{
    static U1 tf_start_sol_last = OFF;

    if (f_start == tf_start_sol_last) {
        if (f_start == ON) {
            f_sol_req = u1_pat_seting[u1_pat_no][EN_PAT_SOL];
        }
        else {
            f_sol_req = OFF;
        }
    }
    tf_start_sol_last = f_start;
}


/* ------------------------ *
 *   ソレノイドドライバ
 * ------------------------ */
void drv_Sol(U1 tu1_ch, U1 tu1_req)
{
    if (u1_sol_seting[tu1_ch][EN_SOL_ON] == ON) {
        if (tu1_req == ON) {
            // Sol Valve Open
            digitalWrite(u1_sol_seting[tu1_ch][EN_SOL_PIN], OPEN);
        }
        else {
            // Sol Valve Close
            digitalWrite(u1_sol_seting[tu1_ch][EN_SOL_PIN], CLOSE);
        }
    }
    else {
        // Sol Valve Close
        digitalWrite(u1_sol_seting[tu1_ch][EN_SOL_PIN], CLOSE);
    }
}


/* ------------------------ *
 *   ソレノイドバルブ出力判定
 * ------------------------ */
void output_Sol(void)
{
    static  U1 tu1_sol_drv[U1_SOL_NUM] = {OFF};
    static  U1 tu1_sol_drv_last[U1_SOL_NUM] = {OFF};
            U1 tf_sol_ena[U1_SOL_NUM] = {DISABLE};
            U1 tf_cnt_clr = OFF;
            U1 tf_cnt_start = OFF;

    if (f_sol_req == ON) {

        for (int i = 0; i < U1_SOL_NUM; i++) {

            tf_sol_ena[i] = judge_Valve_open(i);

            // SOL ON判定
            if (   (tf_sol_ena[i] == ENABLE)
                && (u2_cnt_sol_off[i] == 0)) {

                tu1_sol_drv[i] = ON;

                if (f_synchro_sol == OFF) {
                    f_synchro_sol = ON;
                }
            }

            // SOL OFF判定
            if (tf_sol_ena[i] == DISABLE) {
                tu1_sol_drv[i] = OFF;
            }

            // SOL OFF時間判定
            if (tu1_sol_drv[i] == OFF) {
                u2_cnt_sol_off[i]++;
                tf_cnt_start = ON;

                if (u2_cnt_sol_off[i] > U2_CNT_SOL_OFF) {
                    tf_cnt_clr = ON;
                }
            }
        }

        // 20250424 追加 ここから
        if (tf_cnt_start == ON) {
            for (int i = 0; i < U1_SOL_NUM; i++) {
                tu1_sol_drv[i] = OFF;
            }
        }
        // 20250424 追加 ここまで

        // いずれかのSOL OFF時間が経過した場合、全てのSOLをON
        if (tf_cnt_clr == ON) {
            for (int i = 0; i < U1_SOL_NUM; i++) {
                u2_cnt_sol_off[i] = 0;
            }
            f_synchro_sol = OFF;
        }
    }
    else {
        for (int i = 0; i < U1_SOL_NUM; i++) {
            tu1_sol_drv[i] = OFF;
            u2_cnt_sol_off[i] = 0;
        }
        f_synchro_sol = OFF;
    }


    for (int i = 0; i < U1_SOL_NUM; i++) {
        drv_Sol(i, tu1_sol_drv[i]);
        tu1_sol_drv_last[i] = tu1_sol_drv[i];
    }
}