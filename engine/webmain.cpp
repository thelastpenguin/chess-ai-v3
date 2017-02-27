#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

#include "include/server-http.hpp"

#include "board.hpp"
#include "intelligence.hpp"

#define BOOST_SPIRIT_THREADSAFE

using namespace std;
using namespace boost::property_tree;
using namespace SimpleWeb;

int mode_webui(int port);
int mode_test();


int main(int argc, const char** argv) {
	mode_webui(8080);

    return 0;
}



int mode_webui(int port) {
    typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

    std::cout << "Chess AI by Gareth George" << std::endl;
    std::cout << "\tweb interface loading. port: " << port << std::endl;

    //HTTP-server at port 8080 using 4 threads
    HttpServer server(port, 4);

    server.resource["^/ai$"]["POST"]=[](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        std::cout << "got request to /ai" << std::endl;
        Board board;

        try {
            /*
             read the json request
            */
            ptree pt;
            read_json(request->content, pt);

            /*
             read the current turn
             */
            string currentTurnStr = pt.get<string>("turn");
            TTeam currentTurn = currentTurnStr == "black" ? -1 : 1;

            std::cout << "\tcurrent turn: " << currentTurnStr << std::endl;

            /*
             construct the board game from the json data
             */
            BOOST_FOREACH(ptree::value_type& v, pt.get_child("position"))
            {
                std::cout << "\t" << v.first.data() << ":" << v.second.data() << std::endl;

                string positionStr = boost::lexical_cast<string>(v.first.data());
                string pieceStr = boost::lexical_cast<string>(v.second.data());
                if (positionStr.length() < 2 || pieceStr.length() < 2) {
                    std::cout << "\tskipping piece, malformatted location!" << std::endl;
                    continue ;
                }

                int x = positionStr.c_str()[0] - 'a';
                int y = positionStr.c_str()[1] - '1';

                TTeam team = pieceStr.c_str()[0] == 'b' ? -1 : 1;
                TPiece piece;
                switch (pieceStr.c_str()[1]) {
                case 'P': piece = PIECE_PAWN; break ;
                case 'N': piece = PIECE_KNIGHT; break ;
                case 'B': piece = PIECE_BISHOP; break ;
                case 'R': piece = PIECE_ROOK; break ;
                case 'K': piece = PIECE_KING; break ;
                case 'Q': piece = PIECE_QUEEN; break ;
                default:
                    piece = 0;
                }

                board.setPiece(mailbox64[y * BOARD_DIM + x], piece * team);
            }

            /*
             make a move!
             */

			std::cout << board.toString() << std::endl;
            std::cout << "SCORE: " << board.getScore() << std::endl;

            AIPlayer player(7);
			Move result;
            player.pickBestMove(board, currentTurn, &result);
			Move::TMoveScratchStack stack;
			result.make(board, stack);

			std::cout << "Done computing! Sending result." << std::endl;

            /*
             feed it back
             */
            ptree resTree;
            for (int i = 0; i < BOARD_SIZE; ++i) {
                if (board[mailbox64[i]] == 0) continue;

                std::stringstream posStream;
                posStream << (char) ((i % BOARD_DIM) + 'a') << (char) ((i / BOARD_DIM) + '1');
                std::string posStr = posStream.str();

                std::stringstream pieceStream;
                pieceStream << (board[mailbox64[i]] < 0 ? 'b' : 'w') << pieceGetLetter(board[mailbox64[i]]);
                std::string pieceStr = pieceStream.str();

                resTree.put(posStr, pieceStr);
            }

            std::stringstream out;
            write_json(out, resTree);
            std::string outStr = out.str();

            response << "HTTP/1.1 200 OK\r\nContent-Length: " << outStr.length() << "\r\n\r\n" << outStr;
        }
        catch(exception& e) {
            std::cout << e.what() << std::endl;
            response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n" << e.what();
        }
    };

    server.default_resource["GET"]=[](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        const auto web_root_path=boost::filesystem::canonical("web");
        boost::filesystem::path path=web_root_path;
        path/=request->path;
        if(boost::filesystem::exists(path)) {
            path=boost::filesystem::canonical(path);
            //Check if path is within web_root_path
            if(distance(web_root_path.begin(), web_root_path.end())<=distance(path.begin(), path.end()) &&
               equal(web_root_path.begin(), web_root_path.end(), path.begin())) {
                if(boost::filesystem::is_directory(path))
                    path/="index.html";
                if(boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path)) {
                    ifstream ifs;
                    ifs.open(path.string(), ifstream::in | ios::binary);

                    if(ifs) {
                        ifs.seekg(0, ios::end);
                        size_t length=ifs.tellg();

                        ifs.seekg(0, ios::beg);

                        response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";

                        //read and send 128 KB at a time
                        const size_t buffer_size=131072;
                        vector<char> buffer(buffer_size);
                        size_t read_length;
                        try {
                            while((read_length=ifs.read(&buffer[0], buffer_size).gcount())>0) {
                                response.write(&buffer[0], read_length);
                                response.flush();
                            }
                        }
                        catch(const exception &e) {
                            cerr << "Connection interrupted, closing file" << endl;
                        }

                        ifs.close();
                        return;
                    }
                }
            }
        }
        string content="Could not open path "+request->path;
        response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;

        // return 0;
    };

    std::cout << "\tlaunched server..." << std::endl;
    server.start();

    return 0;
};
