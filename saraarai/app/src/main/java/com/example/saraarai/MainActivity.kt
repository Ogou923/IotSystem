package com.example.saraarai

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import com.example.saraarai.R
import kotlinx.coroutines.*
import java.net.Socket
import java.net.InetSocketAddress
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.PrintWriter
import java.io.OutputStreamWriter
import java.nio.charset.StandardCharsets

class MainActivity : AppCompatActivity() {
    private lateinit var maingamen: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main)
        maingamen = findViewById(R.id.maingamen)

        // 非同期処理を開始
        CoroutineScope(Dispatchers.IO).launch {
            connectAndStartClient()
        }
    }


    private suspend fun connectAndStartClient() {
        val host = "10.0.2.2"
        val port = 8002
        var clientSocket: Socket? = null
        var connected = false

        // 接続を試行するループ（最大20回試行）
        var i = 0
        while (i < 20) {
            try {
                clientSocket = Socket()
                val socketAddress = InetSocketAddress(host, port)
                clientSocket!!.connect(socketAddress, 3000) // 3秒でタイムアウトするように設定

                // 接続成功
                connected = true
                withContext(Dispatchers.Main) { // Dispatchers.MainでしかUIの更新不可（別スレッドのため）
                    maingamen.text = "接続成功"
                }
                break
            } catch (e: Exception) {
                // エラーが発生した場合は待機して再試行
                withContext(Dispatchers.Main) {
                    maingamen.text = "サーバに\n接続中..."
                }
                i++
                delay(1000)
            }
        }

        if (!connected) {
            // 接続失敗時の処理
            withContext(Dispatchers.Main) {
                maingamen.text = "サーバの接続に\n失敗しました"
            }
        }

        // 接続成功したら通信を開始
        clientSocket?.use { socket ->
            val inputReader = BufferedReader(InputStreamReader(socket.getInputStream()))
            val outputWriter = PrintWriter(OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8), true)

            try {
                while (true) {
                    // 受信可能なデータがあるか確認
                    if (inputReader.ready()) {
                        // サーバーからのメッセージを受信
                        val received = withContext(Dispatchers.IO) {
                            inputReader.readLine() // 読み取るTextの末尾が\nでなければならない
                        } ?: break

                        val data = received.split(",")
                        if (data.size == 2) {
                            val (data1, data2) = data // data1=検知したかしてないか、data2=方向
                            // メッセージに基づいて応答を送信
                            when (data1) {
                                "1" -> {
                                    withContext(Dispatchers.IO) {
                                        outputWriter.println("success")
                                    }
                                    // なんか処理
                                    withContext(Dispatchers.Main) {
                                        maingamen.text = "検知した"
                                    }
                                }

                                "2" -> {
                                    withContext(Dispatchers.IO) {
                                        outputWriter.println("success")
                                    }
                                    // なんか処理
                                    withContext(Dispatchers.Main) {
                                        maingamen.text = "検知してない"
                                    }
                                }

                                "quit" -> {
                                    withContext(Dispatchers.IO) {
                                        outputWriter.println("quit")
                                    }
                                    break
                                }

                                "who" -> {
                                    withContext(Dispatchers.IO) {
                                        outputWriter.println("client2")
                                    }
                                }
                            }
                        }
                    }

                    // 1秒の遅延を追加（任意）
                    delay(1000)
                }
            } catch (e: Exception) {
                e.printStackTrace()
            } finally {
                clientSocket!!.close()
                withContext(Dispatchers.Main) {
                    maingamen.text = "通信終了"
                }
            }
        }

    }
}
