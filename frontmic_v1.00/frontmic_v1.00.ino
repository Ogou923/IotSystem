#include <M5Core2.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <arduinoFFT.h>
#include "time.h"
#include <sys/time.h>
 
 
//const char* ssid = "IODATA-fa4fbc-2G";
//const char* password = "8985469686238";
//const char* server_ip = "192.168.0.8";
 
// const char* ssid = "D821DAA9CF6D-2G";
// const char* password = "fd7ya7sb3b5hxa";
// const char* server_ip = "192.168.3.26";
 
const char* ssid = "NKZ-AP";
const char* password = "nkzap-01";
const char* server_ip = "10.77.97.194";
 
//const char* ssid = "TT";
//const char* password = "tomoaki0722";
//const char* server_ip = "192.168.246.193";
 
const int server_port = 8002;
WiFiClient client;
 
// 入力ピンの宣言と初期化
int Analog_Eingang = 33; // アナログ入力ピン
//int Digital_Eingang = 32; // デジタル入力ピン
const int BUFFER_SIZE = 256; // バッファサイズ
 
// 変数の宣言
int16_t micData[BUFFER_SIZE];
float dBData[BUFFER_SIZE];
double vReal[BUFFER_SIZE]; // FFTのための実数部分
double vImag[BUFFER_SIZE]; // FFTのための虚数部分
int peak = 0;
double senddata[4];
int maxIndex = -1;
int high_count = 0;
int low_count = 0;
double timestamps[BUFFER_SIZE]; // タイムスタンプを保存
double frequency[BUFFER_SIZE / 2]; // 周波数を格納する配列
double amplitude[BUFFER_SIZE / 2]; // 振幅スペクトルを格納する配列
double SamplingRate = 10666.67;
 
 
//時間設定
struct timeval start_time;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 9; // 日本のGMTオフセットは9時間
const int   daylightOffset_sec = 0;
//struct timeval tv;
//struct timezone tz;
 
// FFTの設定
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BUFFER_SIZE, SamplingRate); // サンプリング周波数は40kHz
 
 
void setup() {
  M5.begin();
  M5.Lcd.setTextSize(5);
  M5.Lcd.print("front");
  pinMode(Analog_Eingang, INPUT);
  //pinMode(Digital_Eingang, INPUT);
 
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
 
  // IPアドレスを表示
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  //時間設定
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //NTPサーバーチェック
  struct timeval now;
  while (true) {
    gettimeofday(&now, NULL);
    if (now.tv_sec > 0) break; // NTPサーバーと同期が完了したら終了
    delay(100);
    Serial.print(".");
  }
  Serial.println("NTP・・・OK");
}
 
//文字列送信用
void text_sending(const String& message, WiFiClient& client) {
  String msg = message + "\n";
  client.print(msg);
}
 
//配列送信用
void data_sending(const uint8_t* data, size_t dataSize, WiFiClient& client) {
  client.write(data, dataSize);
}
 
String receiving(WiFiClient& client) {
  String data = "";
  while(data == ""){
    data = client.readStringUntil('\n');
    data.trim(); // Remove any trailing newline characters
  }
  return data;
}
 
void server_connect() {
  while (!client.connect(server_ip, server_port)) {
    Serial.println("接続中");
    Serial.print("Server IP: ");
    Serial.println(server_ip);
    Serial.print("Server Port: ");
    Serial.println(server_port);
    delay(3000);
  }
}
 
void soundread() {
    struct timeval current_time;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        micData[i] = analogRead(Analog_Eingang);
        // 現在時刻を取得(経過時間)
        gettimeofday(&current_time, NULL);
        // 経過時間を計算
        double elapsed_time = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_usec - start_time.tv_usec) / 1e6;
        Serial.println(elapsed_time);
        // タイムスタンプを記録
        timestamps[i] = elapsed_time;
    }
}

 
// dB変換関数
void convertToDecibels() {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    float absValue = abs(micData[i]);
    if (absValue == 0) {
      absValue = 1; // 適切な小さな値に置き換える
    }
    dBData[i] = 20 * log10(absValue);
  }
}

// FFTを実行し、周波数スペクトルを表示する関数
void performFFT() {
  // 実数・虚数データの準備
  for (int i = 0; i < BUFFER_SIZE; i++) {
    vReal[i] = micData[i];
    vImag[i] = 0; // 虚数部分は0にする
  }

  // FFT計算
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); // 窓関数の適用
  FFT.compute(FFT_FORWARD);                        // FFTの計算
  FFT.complexToMagnitude();                        // 振幅スペクトルに変換

  // 周波数と振幅スペクトルを計算して代入
  for (int i = 0; i < BUFFER_SIZE / 2; i++) { // Nyquist周波数まで
    double hzdata = i * SamplingRate / BUFFER_SIZE;
    frequency[i] = hzdata;
    amplitude[i] = vReal[i];                    // 振幅スペクトル
  }
}

// バンドパスフィルタを適用する関数
void applyBandPassFilter(double lowCutoff, double highCutoff) {
  for (int i = 0; i < BUFFER_SIZE / 2; i++) {
    if (frequency[i] < lowCutoff || frequency[i] > highCutoff) {
     frequency[i] = 0;
    }
  }
}
 
//謎の関数
bool isSendDataEmpty(double* data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (data[i] != 0.0) {
      return false;
    }
  }
  return true;
}
 
void loop() {
  server_connect();
  String message = receiving(client);
 
  Serial.println("Received: " + message);
  String state;
  if (message == "who,") {
    text_sending("front", client);
    Serial.println("サーバ接続成功");
    // 基準時刻を取得
    gettimeofday(&start_time, NULL);
    Serial.printf("Start time: %ld.%06ld\n", start_time.tv_sec, start_time.tv_usec);
  }
  while(true){
    state = receiving(client);//actionかstopかquit
    while (state == "action") {
      soundread();
      //convertToDecibels(); // dB変換の関数呼び出し
     
      performFFT(); // FFTの関数呼び出し
 
      // HZデータの表示
      Serial.println("Hz Data:");
      for (int i = 0; i < BUFFER_SIZE / 2; i++) {
        Serial.print(frequency[i]);
        if (i < (BUFFER_SIZE / 2) - 1) {
          Serial.print(": ");
        }
      }
      Serial.println();

      // 振幅スペクトルデータの表示
      Serial.println("振幅スペクトル:");
      for (int i = 0; i < BUFFER_SIZE / 2; i++) {
        Serial.print(amplitude[i]);
        if (i < (BUFFER_SIZE / 2) - 1) {
          Serial.print(": ");
        }
      }
      Serial.println();
 
      for (int i = 0; i < BUFFER_SIZE; i++) {
        Serial.print(timestamps[i]);
        if (i < BUFFER_SIZE - 1) {
          Serial.print(" : ");
        }
      }
      Serial.println();
 
   
      // フィルタリングの関数呼び出し
      applyBandPassFilter(700, 1100); // 例: 取得範囲の最低値と最大値を指定
     
      double check = 0.0;
      peak = -1;

      for (int i = 3; i < BUFFER_SIZE / 2; i++) { // 最初の3つを無視
        if (frequency[i] != 0.0){ // 指定範囲内で振幅スペクトルが最大のインデックスを取得
          if (amplitude[i] > check) {
            check = amplitude[i]; // 最大値を更新
            peak = i;             // 最大値のインデックスを更新
          }
        }
      }

      if (peak == -1) { //peakが更新されなければ最後の時間データを格納
        senddata[0] = 0.0;
        senddata[1] = 0.0;
        senddata[2] = floor(timestamps[BUFFER_SIZE / 2 - 1]);
        senddata[3] = (timestamps[BUFFER_SIZE / 2 - 1] - floor(timestamps[BUFFER_SIZE / 2 - 1])) * 1000;
      }else{
        senddata[0] = frequency[peak];
        senddata[1] = amplitude[peak];
        senddata[2] = floor(timestamps[peak]);
        senddata[3] = (timestamps[peak] - floor(timestamps[peak])) * 1000;
      }
      peak = 0;
 
      for (int i = 0; i < BUFFER_SIZE / 2; i++) {
        Serial.print(frequency[i]);
        if (i < BUFFER_SIZE / 2 - 1) {
          Serial.print(": ");
        }
      }
      Serial.println();
 
      data_sending(reinterpret_cast<uint8_t*>(senddata), sizeof(senddata), client);
      Serial.print("send data:");
      Serial.print(senddata[0]);
      Serial.print("-");
      Serial.print(senddata[1]);
      Serial.print("-");
      Serial.print(static_cast<int>(senddata[2]));
      Serial.print(".");
      Serial.println(static_cast<int>(senddata[3]));
      String data = receiving(client);
      if (data == "stop") {
        state = data;//改善できるかも
      }
      Serial.println("Received: " + data);

      senddata[0] = 0.0;
      senddata[1] = 0.0;
      senddata[2] = 0.0;
      senddata[3] = 0.0;
    }
 
    if(state == "quit"){
      break;
    }
  }
 
  Serial.println("通信終了");
  client.stop();
}