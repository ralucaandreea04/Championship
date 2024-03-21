#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdbool.h> 
#include <curl/curl.h>
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

using namespace std;

#define PORT 2024
#define BUFFER_SIZE 1024

int clienti_activi = 0; //numarul de clienti conectati la server
bool logged_utilizator = false;
bool logged_administrator = false; 
int campionate_active = 0; //pt fiecare client se contorizeaza numarul de campionate pentru a fi valabila comanda de renuntare
int champ_single_elim=0; // datele pentru a identifica mai bine campionatele 
int champ_double_elim=0;

void actualizareStatus(char tip[], char nume[],const char login[]) 
{
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    rapidxml::xml_document<> doc;
    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    if (!rootNode) {
        perror("Eroare: Nu s-a găsit nodul radacina 'database'!\n");
        return;
    }

    rapidxml::xml_node<>* tipNode = nullptr;

    if (strcmp(tip, "utilizator") == 0) 
    {
        tipNode = rootNode->first_node("utilizatori");
    } 
    else if (strcmp(tip, "administrator") == 0) 
    {
        tipNode = rootNode->first_node("administratori");
    }

    if (tipNode) 
    {
        for (rapidxml::xml_node<>* userNode = tipNode->first_node(tip); userNode; userNode = userNode->next_sibling(tip)) 
        {
            rapidxml::xml_node<>* usernameNode = userNode->first_node("username");
            if (usernameNode) {
                string username = usernameNode->value();
                if (username == nume) {
                    rapidxml::xml_node<>* statusNode = userNode->first_node("status");
                    if (statusNode) 
                    {
                        userNode->remove_node(statusNode);
                    }
                    rapidxml::xml_node<>* newStatusNode = doc.allocate_node(rapidxml::node_type::node_element, "status", login);
                    userNode->append_node(newStatusNode);
                }
            }
        }
    }

    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

bool verificare_email(char e[]) 
{
    ///Deschiderea fisierului
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return 1;
    }
    // Citeste continutul XML intr-o variabilă de tip string
    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    // Creează un obiect rapidxml::xml_document
    rapidxml::xml_document<> doc;

    // Parseaza conținutul XML folosind metoda parse<0>
    doc.parse<0>(&xmlContent[0]);
    bool emailExists = false;

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    if (!rootNode) 
    {
        perror("Eroare: Nu s-a găsit nodul radacina 'database'!");
    }

    rapidxml::xml_node<>* utilizatoriNode = rootNode->first_node("utilizatori");
    if (utilizatoriNode) 
    {
        for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
        {
            rapidxml::xml_node<>* emailNode = utilizatorNode->first_node("email");
            if (emailNode) 
            {
                string email = emailNode->value();
                if (email == e) 
                {
                    emailExists = true;
                    break;
                }
            }
        }
    }
    return emailExists;
}

bool verificare_username_utilizator(char uu[])
{
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return 1;
    }
    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    rapidxml::xml_document<> doc;

    doc.parse<0>(&xmlContent[0]);
    bool usernameExists = false;

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    if (!rootNode) 
    {
        perror("Eroare: Nu s-a găsit nodul radacina 'database'!");
    }

    rapidxml::xml_node<>* utilizatoriNode = rootNode->first_node("utilizatori");
    if (utilizatoriNode) 
    {
        for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
        {
            rapidxml::xml_node<>* usernameNode = utilizatorNode->first_node("username");
            if (usernameNode) 
            {
                string username = usernameNode->value();
                if (username == uu) 
                {
                    usernameExists = true;
                    break;
                }
            }
        }
    }
    return usernameExists;
}

void adaugaUtilizator(char u[], char email[]) 
{
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    rapidxml::xml_document<> doc;

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    rapidxml::xml_node<>* utilizatoriNode = rootNode->first_node("utilizatori");

    rapidxml::xml_node<>* utilizatorNode = doc.allocate_node(rapidxml::node_type::node_element, "utilizator");

    rapidxml::xml_node<>* usernameNode = doc.allocate_node(rapidxml::node_type::node_element, "username", u);
    rapidxml::xml_node<>* emailNode = doc.allocate_node(rapidxml::node_type::node_element, "email", email);

    ///status default = logged
    rapidxml::xml_node<>* statusNode = doc.allocate_node(rapidxml::node_type::node_element, "status", "logat"); 

    utilizatorNode->append_node(usernameNode);
    utilizatorNode->append_node(emailNode);
    utilizatorNode->append_node(statusNode);

    utilizatoriNode->append_node(utilizatorNode);

    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

bool verificare_username_administrator(char u[]) 
{
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return false;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    rapidxml::xml_document<> doc;

    doc.parse<0>(&xmlContent[0]);
    bool usernameExists = false;

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    if (!rootNode) {
        perror("Eroare: Nu s-a găsit nodul radacina 'database'!");
    }

    rapidxml::xml_node<>* administratoriNode = rootNode->first_node("administratori");
    if (administratoriNode) {
        for (rapidxml::xml_node<>* administratorNode = administratoriNode->first_node("administrator"); administratorNode; administratorNode = administratorNode->next_sibling("administrator")) {
            rapidxml::xml_node<>* usernameNode = administratorNode->first_node("username");
            if (usernameNode) {
                string username = usernameNode->value();
                if (username == u) {
                    usernameExists = true;
                    break;
                }
            }
        }
    }
    return usernameExists;
}

void adaugaAdministrator(char username[])
{
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    rapidxml::xml_document<> doc;

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    rapidxml::xml_node<>* administratoriNode = rootNode->first_node("administratori");

    rapidxml::xml_node<>* administratorNode = doc.allocate_node(rapidxml::node_type::node_element, "administrator");

    rapidxml::xml_node<>* usernameNode = doc.allocate_node(rapidxml::node_type::node_element, "username", username);

    ///status default = logged
    rapidxml::xml_node<>* statusNode = doc.allocate_node(rapidxml::node_type::node_element, "status", "logat"); 

    administratorNode->append_node(usernameNode);
    administratorNode->append_node(statusNode);

    administratoriNode->append_node(administratorNode);

    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

void adaugaCampionat(char tipCampionat[], int numarPartide, char scor[], char participanti[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    if (!rootNode) 
    {
        perror("Eroare: Nu s-a găsit nodul radacină 'database'!\n");
        return;
    }

    rapidxml::xml_node<>* championshipsNode = rootNode->first_node("championships");
    if (!championshipsNode) 
    {
        championshipsNode = doc.allocate_node(rapidxml::node_type::node_element, "championships");
        rootNode->append_node(championshipsNode);
    }

    rapidxml::xml_node<>* championshipNode = nullptr;
    if (strstr(tipCampionat, "single_elimination") != nullptr) 
    {
        char nodCampionat[50];
        snprintf(nodCampionat, sizeof(nodCampionat), "championship_single_elimination_%d", champ_single_elim);
        championshipNode = doc.allocate_node(rapidxml::node_type::node_element, nodCampionat);
    } 
    else if (strstr(tipCampionat, "double_elimination") != nullptr) 
    {
        char nodCampionat[50];
        snprintf(nodCampionat, sizeof(nodCampionat), "championship_double_elimination_%d", champ_double_elim);
        championshipNode = doc.allocate_node(rapidxml::node_type::node_element, nodCampionat);
    }
    ///Informatii despre campionat (valori aleatorii)
    rapidxml::xml_node<>* informatiiNode = doc.allocate_node(rapidxml::node_type::node_element, "informatii");
    rapidxml::xml_node<>* oraNode = doc.allocate_node(rapidxml::node_type::node_element, "ora");
    oraNode->value(doc.allocate_string("12:00"));
    informatiiNode->append_node(oraNode);

    rapidxml::xml_node<>* dataNode = doc.allocate_node(rapidxml::node_type::node_element, "data");
    dataNode->value(doc.allocate_string("12.01.2024")); 
    informatiiNode->append_node(dataNode);
    championshipNode->append_node(informatiiNode);

    rapidxml::xml_node<>* matchesNode = doc.allocate_node(rapidxml::node_type::node_element, "matches");

    char* urm_scor;
    char* p = strtok_r(scor, "-", &urm_scor);
    char* urm_participant;
    char* t = strtok_r(participanti, "|", &urm_participant);

    int numarPartida = 1; // Inițializează numărul partidei
    while (p != NULL && t != NULL && numarPartide > 0) 
    {
        string numePartida = "Partida" + to_string(numarPartida);

        rapidxml::xml_node<>* matchNode = doc.allocate_node(rapidxml::node_type::node_element, "match");
        rapidxml::xml_node<>* matchNameNode = doc.allocate_node(rapidxml::node_type::node_element, "name");
        matchNameNode->value(doc.allocate_string(numePartida.c_str()));
        matchNode->append_node(matchNameNode);

        rapidxml::xml_node<>* matchScoreNode = doc.allocate_node(rapidxml::node_type::node_element, "score");
        matchScoreNode->value(doc.allocate_string(p));
        matchNode->append_node(matchScoreNode);

        rapidxml::xml_node<>* matchMembersNode = doc.allocate_node(rapidxml::node_type::node_element, "members");
        matchMembersNode->value(doc.allocate_string(t));
        matchNode->append_node(matchMembersNode);

        matchesNode->append_node(matchNode);

        ++numarPartida;
        p = strtok_r(NULL, "-", &urm_scor);
        t = strtok_r(NULL, "|", &urm_participant);
        numarPartide--;
    }

    championshipNode->append_node(matchesNode);
    championshipsNode->append_node(championshipNode);

    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

void reprogrameazaCampionat(char* numeCampionat, char* data_noua, char* ora_noua) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* rootNode = doc.first_node("database");
    if (!rootNode) 
    {
        perror("Eroare: Nu s-a găsit nodul radacină 'database'!\n");
        return;
    }

    char nodCampionat[100]="";
    strcat(nodCampionat,"championship_");
    strcat(nodCampionat,numeCampionat);

    rapidxml::xml_node<>* championshipsNode = rootNode->first_node("championships");
    if (championshipsNode) 
    {
        rapidxml::xml_node<>* championshipNode = championshipsNode->first_node(nodCampionat);
        if (championshipNode) 
        {
            rapidxml::xml_node<>* informatiiNode = championshipNode->first_node("informatii");
            if (informatiiNode) 
            {
                rapidxml::xml_node<>* oraNode = informatiiNode->first_node("ora");
                if (oraNode) 
                {
                    informatiiNode->remove_node(oraNode);
                    rapidxml::xml_node<>* NeworaNode=doc.allocate_node(rapidxml::node_type::node_element, "ora" ,"0");
                    NeworaNode->value(doc.allocate_string(ora_noua));
                    informatiiNode->append_node(NeworaNode);
                } 
                rapidxml::xml_node<>* dataNode = informatiiNode->first_node("data");
                if (dataNode) 
                {
                    informatiiNode->remove_node(dataNode);
                    rapidxml::xml_node<>* NewdataNode=doc.allocate_node(rapidxml::node_type::node_element, "data" ,"0");
                    NewdataNode->value(doc.allocate_string(data_noua));
                    informatiiNode->append_node(NewdataNode);
                }
            }
        }
    }
    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

bool campionateDisponibile(char tipCamp[]) 
{
    rapidxml::xml_document<> doc;
    fstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fișierului pentru citire!\n");
        return false;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* championshipsNode = doc.first_node("database")->first_node("championships");
    if (!championshipsNode) 
    {
        perror("Nu există nodul 'championships' în fișierul XML!\n");
        return false;
    }

    /// realizarea corespunzatoare prin concatenare a nodului care corespunde in fisierul xml
    char nodCampionat[50];
    snprintf(nodCampionat, sizeof(nodCampionat), "championship_%s", tipCamp);

    rapidxml::xml_node<>* championshipNode = championshipsNode->first_node(nodCampionat);

    if (!championshipNode) 
    {
       perror("Nu există nodul pentru campionatul dat în fișierul XML!\n");
        return false;
    }

    rapidxml::xml_node<>* matchesNode = championshipNode->first_node("matches");
    if (!matchesNode) 
    {
        perror("Nu există nodul pentru meciuri în fișierul XML!\n");
        return false;
    }

    int nrLocuri=0;

    for (rapidxml::xml_node<>* matchNode = matchesNode->first_node("match"); matchNode; matchNode = matchNode->next_sibling()) 
    {
        rapidxml::xml_node<>* matchMembersNode = matchNode->first_node("members");
        if (matchMembersNode)
        {
            nrLocuri+= stoi(matchMembersNode->value());
        }
    }

    if(nrLocuri>0) return true;
    else return false;
}

bool participareCampionat(char tip[], char nume[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
        return false;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* utilizatoriNode = doc.first_node("database")->first_node("utilizatori");
    if (!utilizatoriNode) 
    {
        perror("Nu exista nodul 'utilizatori' în fisierul XML!\n");
        return false;
    }

    for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
    {
        rapidxml::xml_node<>* usernameNode = utilizatorNode->first_node("username");
        if (usernameNode && strcmp(usernameNode->value(), nume) == 0) 
        {
            rapidxml::xml_node<>* campionateNode = utilizatorNode->first_node("campionate");
            if (campionateNode) 
            {
                for (rapidxml::xml_node<>* campionatNode = campionateNode->first_node("campionat"); campionatNode; campionatNode = campionatNode->next_sibling("campionat")) 
                {
                    if (strcmp(campionatNode->value(), tip) == 0) 
                    {
                        return true; // Utilizatorul este deja înscris în acest campionat
                    }
                }
            }
        }
    }
    return false; 
}

bool repartizareCampionat(char tipCamp[], char username[]) 
{
    if(campionateDisponibile(tipCamp) && !participareCampionat(tipCamp,username))
    {
        rapidxml::xml_document<> doc;
        fstream file("baza_date.xml");
        if (!file.is_open()) 
        {
            perror("Eroare la deschiderea fisierului pentru citire!\n");
            return false;
        }

        string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        doc.parse<0>(&xmlContent[0]);

        rapidxml::xml_node<>* championshipsNode = doc.first_node("database")->first_node("championships");
        if (!championshipsNode) 
        {
            perror("Nu exista nodul 'championships' în fisierul XML!\n");
            return false;
        }

        char nodCampionat[50];
        snprintf(nodCampionat, sizeof(nodCampionat), "championship_%s", tipCamp);

        rapidxml::xml_node<>* utilizatoriNode = doc.first_node("database")->first_node("utilizatori");
        if (!utilizatoriNode) 
        {
            perror("Nu exista nodul 'utilizatori' în fisierul XML!\n");
            return false;
        }

        rapidxml::xml_node<>* championshipNode = championshipsNode->first_node(nodCampionat);
        if (!championshipNode) 
        {
            perror("Nu exista nodul pentru campionatul dat în fisierul XML!\n");
            return false;
        }

        rapidxml::xml_node<>* matchesNode = championshipNode->first_node("matches");
        if (!matchesNode) 
        {
            perror("Nu exista nodul pentru meciuri în fisierul XML!\n");
            return false;
        }

        bool campionatGasit = false;

        for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
        {
            rapidxml::xml_node<>* usernameNode = utilizatorNode->first_node("username");
            if (usernameNode && strcmp(usernameNode->value(), username) == 0) 
            {
                rapidxml::xml_node<>* campionateNode = utilizatorNode->first_node("campionate");
                if (!campionateNode) 
                {
                    campionateNode = doc.allocate_node(rapidxml::node_type::node_element, "campionate");
                    utilizatorNode->append_node(campionateNode);
                }

                for (rapidxml::xml_node<>* campionatNode = campionateNode->first_node("campionat"); campionatNode; campionatNode = campionatNode->next_sibling("campionat")) 
                {
                    if (strcmp(campionatNode->value(), tipCamp) == 0) 
                    {
                        campionatGasit = true;
                        break;
                    }
                }

                if (!campionatGasit) 
                {
                    rapidxml::xml_node<>* nouCampionatNode = doc.allocate_node(rapidxml::node_type::node_element, "campionat", tipCamp);
                    nouCampionatNode->value(tipCamp);
                    campionateNode->append_node(nouCampionatNode);
                }

                rapidxml::xml_node<>* matchNode = matchesNode->first_node("match");

                for (rapidxml::xml_node<>* campionatNode = campionateNode->first_node("campionat"); campionatNode; campionatNode = campionatNode->next_sibling("campionat")) 
                {
                    if (strcmp(campionatNode->value(), tipCamp) == 0) 
                    {
                        rapidxml::xml_node<>* punctajNode = utilizatorNode->first_node("punctaj");
                        if (!punctajNode)
                        {
                            punctajNode = doc.allocate_node(rapidxml::node_type::node_element, "punctaj");
                            punctajNode->value(doc.allocate_string("0")); 
                            utilizatorNode->append_node(punctajNode);
                        }

                        int currentScore = atoi(punctajNode->value());
                        int matchScore = atoi(matchNode->first_node("score")->value());
                        
                        utilizatorNode->remove_node(punctajNode);

                        // scorul actualizat
                        currentScore += matchScore;

                        // conversia scorului actualizat înapoi în șir pentru a actualiza nodul punctaj
                        rapidxml::xml_node<>* NpunctajNode = doc.allocate_node(rapidxml::node_type::node_element, "punctaj");
                        string newScore = to_string(currentScore);
                        NpunctajNode->value(doc.allocate_string(newScore.c_str()));
                        utilizatorNode->append_node(NpunctajNode);
                        break;
                    }
                }
            }
        }

        for (rapidxml::xml_node<>* matchNode = matchesNode->first_node("match"); matchNode; matchNode = matchNode->next_sibling()) 
        {
            rapidxml::xml_node<>* matchMembersNode = matchNode->first_node("members");
            if (matchMembersNode) 
            {
                int currentMembers = stoi(matchMembersNode->value());
                if (currentMembers > 0) 
                {
                    rapidxml::xml_node<>* clientNode = doc.allocate_node(rapidxml::node_type::node_element, "membru");
                    clientNode->value(username);
                    matchNode->remove_node(matchMembersNode);

                    rapidxml::xml_node<>* members = doc.allocate_node(rapidxml::node_type::node_element, "members");

                    currentMembers--;
                    members->value(to_string(currentMembers).c_str());

                    matchNode->append_node(clientNode);
                    matchNode->append_node(members);

                    string xmlAsString;
                    rapidxml::print(back_inserter(xmlAsString), doc);

                    ofstream fileOutput("baza_date.xml");
                    fileOutput << xmlAsString;
                    fileOutput.close();
                    return true;
                }
            }
        }
    }
    return false;
}

void eliminaUtilizatorDinCampionate(char numeUtilizator[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");
    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);
    rapidxml::xml_node<>* championshipsNode = doc.first_node("database")->first_node("championships");
    
    for (rapidxml::xml_node<>* campionatNode = championshipsNode->first_node(); campionatNode; campionatNode = campionatNode->next_sibling()) 
    { 
        rapidxml::xml_node<>* matchesNode = campionatNode->first_node("matches");
        for (rapidxml::xml_node<>* matchNode = matchesNode->first_node("match"); matchNode; matchNode = matchNode->next_sibling("match")) 
        {
            rapidxml::xml_node<>* membriNode = matchNode->first_node("membru");
            while (membriNode) 
            {
                rapidxml::xml_node<>* nextMembriNode = membriNode->next_sibling("membru");
                if (strcmp(membriNode->value(), numeUtilizator) == 0) 
                {
                    matchNode->remove_node(membriNode);
                    rapidxml::xml_node<>* nrMembriNode = matchNode->first_node("members");
                    if (nrMembriNode) 
                    {
                        int nrMembri = atoi(nrMembriNode->value());
                        nrMembri++;
                        string nr = to_string(nrMembri);

                        matchNode->remove_node(nrMembriNode);

                        rapidxml::xml_node<>* newNrMembriNode = doc.allocate_node(rapidxml::node_type::node_element, "members", nr.c_str());
                        matchNode->append_node(newNrMembriNode);
                    }
                    break;
                }
                membriNode = nextMembriNode;
            }
        }
    }
    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

void resetare_client(char nume[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");
    if (!file.is_open()) {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* utilizatoriNode = doc.first_node("database")->first_node("utilizatori");
    if (!utilizatoriNode) 
    {
        printf("Nu există nodul 'utilizatori' în fișierul XML!\n");
        return;
    }

    rapidxml::xml_node<>* administratoriNode = doc.first_node("database")->first_node("administratori");
    if (!administratoriNode) 
    {
        printf("Nu există nodul 'administratori' în fișierul XML!\n");
        return;
    }

    bool gasit = false;
    // pentru utilizatori, actualizăm statusul și punctajul
    for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode && !gasit; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
    {
        rapidxml::xml_node<>* usernameNode = utilizatorNode->first_node("username");
        if (usernameNode && strcmp(usernameNode->value(), nume) == 0) 
        {
            rapidxml::xml_node<>* statusNode = utilizatorNode->first_node("status");
            if (statusNode) 
            {
                utilizatorNode->remove_node(statusNode);
                rapidxml::xml_node<>* NstatusNode=doc.allocate_node(rapidxml::node_type::node_element, "status", "nu_sunt_logat");
                utilizatorNode->append_node(NstatusNode);
            }

            rapidxml::xml_node<>* campionateNode = utilizatorNode->first_node("campionate");
            if (campionateNode) 
            {
                utilizatorNode->remove_node(campionateNode); 
            }
            gasit = true;
            break;
        }
    }

    // pentru administratori, actualizam doar statusul
    for (rapidxml::xml_node<>* administratorNode = administratoriNode->first_node("administrator"); administratorNode && !gasit; administratorNode = administratorNode->next_sibling("administrator")) 
    {
        rapidxml::xml_node<>* usernameNode = administratorNode->first_node("username");
        if (usernameNode && strcmp(usernameNode->value(), nume) == 0) 
        {
            rapidxml::xml_node<>* statusNode = administratorNode->first_node("status");
            if (statusNode && strcmp(statusNode->value(), "logat") == 0) 
            {
                administratorNode->remove_node(statusNode);
                rapidxml::xml_node<>* status=doc.allocate_node(rapidxml::node_type::node_element, "status", "nu_sunt_logat");
                administratorNode->append_node(status);
            }
            break; 
        }
    }

    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

void renuntaLaCampionat(char numeUtilizator[], char numeCampionat[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");

    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* utilizatoriNode = doc.first_node("database")->first_node("utilizatori");
    rapidxml::xml_node<>* championshipsNode = doc.first_node("database")->first_node("championships");

    if (!utilizatoriNode || !championshipsNode) {
        printf("Eroare: Structura XML invalidă!\n");
        return;
    }

    for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
    {
        rapidxml::xml_node<>* usernameNode = utilizatorNode->first_node("username");
        rapidxml::xml_node<>* campionateNode = utilizatorNode->first_node("campionate");
        rapidxml::xml_node<>* punctajNode = utilizatorNode->first_node("punctaj");

        if (strcmp(usernameNode->value(), numeUtilizator) == 0) 
        {
            for (rapidxml::xml_node<>* campionatUtilizatorNode = campionateNode->first_node("campionat"); campionatUtilizatorNode; campionatUtilizatorNode = campionatUtilizatorNode->next_sibling("campionat")) 
            {
                if (strcmp(campionatUtilizatorNode->value(), numeCampionat) == 0) 
                {
                    int punctaj_utilizator = atoi(punctajNode->value());
                    campionateNode->remove_node(campionatUtilizatorNode);

                    for (rapidxml::xml_node<>* campionatNode = championshipsNode->first_node(); campionatNode; campionatNode = campionatNode->next_sibling()) 
                    {
                        if (strcmp(campionatNode->name(), ("championship_" + string(numeCampionat)).c_str()) == 0) 
                        {
                            rapidxml::xml_node<>* matchesNode = campionatNode->first_node("matches");
                            rapidxml::xml_node<>* matchNode = matchesNode->first_node("match");
                            rapidxml::xml_node<>* punctajCampionatNode = matchNode->first_node("score");
                            if (punctajCampionatNode) 
                            {
                                int punctajCampionat = atoi(punctajCampionatNode->value());
                                punctaj_utilizator -= punctajCampionat;
                                string new_punctaj = to_string(punctaj_utilizator);
                                if (punctajNode) 
                                {
                                    utilizatorNode->remove_node(punctajNode);
                                    rapidxml::xml_node<>* newPunctajNode = doc.allocate_node(rapidxml::node_type::node_element, "punctaj");
                                    newPunctajNode->value(doc.allocate_string(new_punctaj.c_str()));
                                    utilizatorNode->append_node(newPunctajNode);
                                } 
                                else 
                                {
                                    rapidxml::xml_node<>* newPunctajNode = doc.allocate_node(rapidxml::node_type::node_element, "punctaj");
                                    newPunctajNode->value(doc.allocate_string(new_punctaj.c_str()));
                                    utilizatorNode->append_node(newPunctajNode);
                                }

                                break;
                            }
                        }
                    }
                    if (!campionateNode->first_node()) {
                        utilizatorNode->remove_node(campionateNode);
                    }
                    break;
                }
            }
            break;
        }
    }

    string xmlAsString;
    rapidxml::print(back_inserter(xmlAsString), doc);

    ofstream fileOutput("baza_date.xml");
    fileOutput << xmlAsString;
    fileOutput.close();
}

void elimina_de_la_un_campionat(char tip[], char username[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");

    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
        return;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* championshipsNode = doc.first_node("database")->first_node("championships");

    if (!championshipsNode) 
    {
        perror("Nu exista nodul pentru championships!\n");
        return;
    }

    for (rapidxml::xml_node<>* campionatNode = championshipsNode->first_node(); campionatNode; campionatNode = campionatNode->next_sibling())
    {
        if (strcmp(campionatNode->name(), ("championship_" + string(tip)).c_str()) == 0)
        {
            rapidxml::xml_node<>* matchesNode = campionatNode->first_node("matches");
            if (!matchesNode) 
            {
                perror("Eroare: Structura campionatului este invalida : nu exista nodul matches!\n");
                return;
            }
            for (rapidxml::xml_node<>* matchNode = matchesNode->first_node("match"); matchNode; matchNode = matchNode->next_sibling("match")) 
            {
                rapidxml::xml_node<>* membriNode = matchNode->first_node("membru");
                if (membriNode && strcmp(membriNode->value(), username) == 0) 
                {
                    matchNode->remove_node(membriNode);

                    rapidxml::xml_node<>* nrMembriNode = matchNode->first_node("members");
                    if (nrMembriNode) 
                    {
                        int nrMembri = atoi(nrMembriNode->value());
                        nrMembri++;
                        string nr = to_string(nrMembri);

                        matchNode->remove_node(nrMembriNode);
                        rapidxml::xml_node<>* newNrMembriNode = doc.allocate_node(rapidxml::node_type::node_element, "members", nr.c_str());
                        matchNode->append_node(newNrMembriNode);
                    }
                    
                    string xmlAsString;
                    rapidxml::print(back_inserter(xmlAsString), doc);

                    ofstream fileOutput("baza_date.xml");
                    fileOutput << xmlAsString;
                    fileOutput.close();
                    return;
                }
            }
            return;
        }
    }
}

vector<string> vizualizarePunctaje() 
{
    vector<string> punctaje;

    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");

    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
        return punctaje;
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* utilizatoriNode = doc.first_node("database")->first_node("utilizatori");

    if (!utilizatoriNode) 
    {
        perror("Eroare: nu se poate gasi nodul pentru utilizatori!\n");
        return punctaje;
    }

    for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
    {
        rapidxml::xml_node<>* numeNode = utilizatorNode->first_node("username");
        rapidxml::xml_node<>* punctajNode = utilizatorNode->first_node("punctaj");
        
        if (numeNode && punctajNode) 
        {
            string punctaj = "\n" + string(numeNode->value()) + " : " + string(punctajNode->value());
            punctaje.push_back(punctaj);
        }
        else if(numeNode && !punctajNode)
        {
            string punctaj = "\n" + string(numeNode->value()) + " : nu a fost inregistrat niciodata la vreun campionat";
            punctaje.push_back(punctaj);
        }
    }
    punctaje.push_back("\n");
    return punctaje;
}

string afisareCampionate() 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");

    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    string camp;

    rapidxml::xml_node<>* championshipsNode = doc.first_node("database")->first_node("championships");

    if (!championshipsNode) 
    {
        perror("Nu există nodul 'championships' în fișierul XML.\n");
    }

    for (rapidxml::xml_node<>* championshipNode = championshipsNode->first_node(); championshipNode; championshipNode = championshipNode->next_sibling()) 
    {
        camp += string(championshipNode->name()) + ":\n";
        rapidxml::xml_node<>* matchesNode = championshipNode->first_node("matches");

        if (!matchesNode) 
        {
            camp += "Nu există meciuri pentru acest campionat.\n";
            continue;
        }

        for (rapidxml::xml_node<>* matchNode = matchesNode->first_node("match"); matchNode; matchNode = matchNode->next_sibling("match")) 
        {
            rapidxml::xml_node<>* nameNode = matchNode->first_node("name");
            rapidxml::xml_node<>* scoreNode = matchNode->first_node("score");

            if (nameNode && scoreNode) 
            {
                camp += "Nume: " + string(nameNode->value()) + ", Scor: " +string(scoreNode->value()) + "\n";
            }
        }
    }
    return camp;
}

string vizualizare_profil_utilizator(char username[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");

    if (!file.is_open()) {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
        return "Eroare la citirea fisierului";
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);
    rapidxml::xml_node<> *root = doc.first_node("database");

    if (!root) {
        return "Eroare: Nu s-a gasit elementul radacina 'database'\n";
    }

    string result;

    for (rapidxml::xml_node<> *utilizator = root->first_node("utilizatori")->first_node("utilizator"); utilizator; utilizator = utilizator->next_sibling()) 
    {
        rapidxml::xml_node<> *usernameNode = utilizator->first_node("username");
        if (usernameNode && strcmp(usernameNode->value(), username) == 0) 
        {
            rapidxml::xml_node<> *emailNode = utilizator->first_node("email");
            rapidxml::xml_node<> *punctajNode = utilizator->first_node("punctaj");
            rapidxml::xml_node<> *statusNode = utilizator->first_node("status");

            result += "Email: " + (emailNode ? string(emailNode->value()) : "") + "\n";
            result += "Punctaj: " + (punctajNode ? string(punctajNode->value()) : "") + "\n";
            result += "Status: " + (statusNode ? string(statusNode->value()) : "") + "\n";

            rapidxml::xml_node<> *campionateNode = utilizator->first_node("campionate");
            if (campionateNode) 
            {
                result += "Campionate: ";
                for (rapidxml::xml_node<> *campionat = campionateNode->first_node("campionat"); campionat; campionat = campionat->next_sibling()) 
                {
                    result += string(campionat->value()) + " ";
                }
                result += "\n";
            }
        }
    }
    return result;
}

char* preluare_email(char username[]) 
{
    rapidxml::xml_document<> doc;
    ifstream file("baza_date.xml");

    if (!file.is_open()) 
    {
        perror("Eroare la deschiderea fisierului pentru citire!\n");
    }

    string xmlContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    doc.parse<0>(&xmlContent[0]);

    rapidxml::xml_node<>* utilizatoriNode = doc.first_node("database")->first_node("utilizatori");
    if (utilizatoriNode) 
    {
        for (rapidxml::xml_node<>* utilizatorNode = utilizatoriNode->first_node("utilizator"); utilizatorNode; utilizatorNode = utilizatorNode->next_sibling("utilizator")) 
        {
            rapidxml::xml_node<>* usernameNode = utilizatorNode->first_node("username");
            rapidxml::xml_node<>* emailNode = utilizatorNode->first_node("email");
            if (usernameNode && emailNode && string(usernameNode->value()) == string(username)) 
            {
                return emailNode->value();
            }
        }
    }
    return nullptr;
}

struct upload_status 
{
    int lines_read;
    const char *data;
    size_t data_size;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) 
{
    FILE *file = (FILE*)userp;
    size_t retcode = fread(ptr, size, nmemb, file);
    return retcode;
}

void trimitere_email(char email[])
{
    CURL *curl;
    CURLcode res = CURLE_OK;
    FILE *file = fopen("test_email.txt", "rb");

    if (!file) 
    {
        printf("Eroare la deschidea fisierului!\n");
        return;
    }

    curl = curl_easy_init();
    if (curl) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_USERNAME, "ralucaandreea313@gmail.com");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "cpycedfdnjxauybt");
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "ralucaandreea313@gmail.com");

        struct curl_slist *recipients = NULL;
        recipients = curl_slist_append(recipients, email);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, file);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        res = curl_easy_perform(curl);

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }

    fclose(file);

    if (res != CURLE_OK) {
        printf("Mesaj esuat!\n");
    } else {
        printf("Mesaj trimis!\n");
    }
}

void client(int client_socket) 
{
    char buffer[BUFFER_SIZE];
    int bytes_received;
    bool exit1 = false;

    // Mesaj de inceput
    char msg[] = "Conectare reusita la server!\n";
    send(client_socket, msg, strlen(msg), 0);

    while (!exit1) 
    {
        char username[50];
        // Primirea comenzii de la client
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) 
        {
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Comanda primita de la client: %s", buffer);

        ///QUIT 
        if (strncmp(buffer, "quit", 4) == 0) 
        {
            resetare_client(username);
            eliminaUtilizatorDinCampionate(username);
            char quit_msg[] = "Conexiune inchisa de server.\n";
            send(client_socket, quit_msg, strlen(quit_msg), 0);
            clienti_activi--;
            exit1 = true;
        }

        ///LOGOUT
        if (strncmp(buffer, "logout", 6) == 0 && (logged_administrator || logged_utilizator)) 
        {
            clienti_activi--;
            resetare_client(username);
            if(verificare_username_utilizator(username))
                { 
                    eliminaUtilizatorDinCampionate(username); 
                    logged_utilizator=false;
                }
            else if(verificare_username_administrator(username))
                logged_administrator=false;
            char logout_msg[] = "Te-ai delogat cu succes!\n";
            send(client_socket, logout_msg, strlen(logout_msg), 0);
        }

        ///UTILIZATOR
        else if (!logged_utilizator && strncmp(buffer, "inscriere utilizator : ", 23) == 0) 
        {
            char email[100];
            sscanf(buffer, "inscriere utilizator : %s , %s", username, email);
            if(verificare_email(email) && verificare_username_utilizator(username)) 
            {
                char confirmation_msg[] = "Esti deja logat! Pentru a continua trebuie sa te autentifici.\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                logged_utilizator=false;
            }
            else if(!verificare_email(email) && !verificare_username_utilizator(username))
            {
                string campionate = afisareCampionate();
                adaugaUtilizator(username,email);
                char confirmation_msg[] = "Utilizatorul a fost inregistrat cu succes!\nAcestea sunt campionatele la care te poti inscrie:\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                send(client_socket, campionate.c_str(), campionate.length(), 0);
                logged_utilizator=true;
            }
            else 
            {
                char confirmation_msg[] = "Datele trimise nu sunt valide.\n Verificati adresa de email sau username-ul.\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                logged_utilizator=false;
            }
        } 
        else if (!logged_utilizator && strncmp(buffer, "autentificare utilizator : ", 27) == 0) 
        {
            char tip[]="utilizator";
            sscanf(buffer, "autentificare utilizator : %s", username);
            if(verificare_username_utilizator(username)) 
            {
                string campionate = afisareCampionate();
                char confirmation_msg[] = "Te-ai autentificat cu succes!\nAcestea sunt campionatele la care te poti inscrie:\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                send(client_socket, campionate.c_str(), campionate.length(), 0);
                logged_utilizator=true;
                actualizareStatus(tip,username,"logat");
            }
            if(!verificare_username_utilizator(username))
            {
                char msg[]="Datele sunt invalide! Verifica username-ul!";
                send(client_socket, msg, strlen(msg), 0);
            }
        }
        else if (logged_utilizator && strncmp(buffer, "inscriere campionat : ", 22) == 0) 
        {
            char tip[BUFFER_SIZE];
            sscanf(buffer, "inscriere campionat : %s", tip);
            if(repartizareCampionat(tip,username))
            {
                const char* email_user= preluare_email(username);
                if (email_user != nullptr) 
                {
                    char email[100];
                    strcpy(email, email_user);
                    trimitere_email(email);
                }
                else 
                {
                    send(client_socket, "Email invalid!\n", strlen("Email invalid!\n"), 0);
                }
                char confirmation_msg[] = "Te-ai scris la acest campionat.\nPentru mai multe detalii te rog sa verifici email-ul!\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                campionate_active++;
            }
            else if(!repartizareCampionat(tip,username))
            {
                char msg[]="Campionatul la care doresti nu mai este disponibil!\nIncearca mai tarziu sau la un alt tip de campionat!\n";
                send(client_socket, msg, strlen(msg), 0);
            }
        } 
        else if (logged_utilizator && campionate_active > 0 && strncmp(buffer, "renuntare campionat ", 20) == 0) 
        {
            char tip[BUFFER_SIZE];
            sscanf(buffer, "renuntare campionat : %s", tip);
            renuntaLaCampionat(username,tip);
            elimina_de_la_un_campionat(tip,username);
            char confirmation_msg[] = "Campionatul este sterg din lista ta.\n";
            send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
            campionate_active--;
        }
        else if(logged_utilizator && strncmp(buffer, "reprogramare campionat : ", 25) == 0) 
        {
            char tip[100], ora[100], data[100];
            sscanf(buffer, "reprogramare campionat : %s , %s , %s", tip, ora, data);
            reprogrameazaCampionat(tip, data, ora);
            char msg[BUFFER_SIZE];
            sprintf(msg, "Campionatul a fost reprogramat de %s!\n", username);
            send(client_socket, msg, strlen(msg), 0);
        }
        else if(logged_utilizator && strncmp(buffer,"vizualizare profil",18)==0)
        {
            string profil=vizualizare_profil_utilizator(username);
            char msg[]="Acesta este profilul tau:\n";
            send(client_socket,msg,strlen(msg),0);
            send(client_socket,profil.c_str(),profil.length(),0);
        }

        // ADMINISTRATOR
        else if (!logged_administrator && strncmp(buffer, "inscriere administrator : ", 28) == 0) 
        {
            sscanf(buffer, "inscriere administrator : %s", username);
            if(verificare_username_administrator(username)) 
            {
                char confirmation_msg[] = "Esti deja logat! Pentru a continua trebuie sa te autentifici.\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                logged_administrator=false;
            }
            else if(!verificare_username_administrator(username))
            {
                adaugaAdministrator(username);
                char confirmation_msg[] = "Administratorul a fost inregistrat si logat cu succes!\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                logged_administrator=true;
            }
            else 
            {
                char confirmation_msg[] = "Datele trimise nu sunt valide. Verificati username-ul.\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                logged_administrator=false;
            }
        }
        else if (!logged_administrator && strncmp(buffer, "autentificare administrator : ", 30) == 0) 
        {
            char tip[BUFFER_SIZE]="administrator";
            sscanf(buffer, "autentificare administrator : %s", username);
            if(verificare_username_administrator(username)) 
            {
                char confirmation_msg[] = "Te-ai autentificat cu succes!\n";
                send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
                logged_administrator=true;
                actualizareStatus(tip,username,"logat");
            }
        }
        else if (logged_administrator && strncmp(buffer, "inregistrare campionat : ", 25) == 0) 
        {
            char tip[50];
            int nr_partide=0;
            char punctaj[BUFFER_SIZE];
            char participanti[BUFFER_SIZE];
            sscanf(buffer, "inregistrare campionat : %s , %d , %s , %s", tip,&nr_partide,punctaj,participanti);

            if (strstr(tip, "single_elimination") != nullptr) 
                champ_single_elim++;
            else if (strstr(tip, "double_elimination") != nullptr) 
                champ_double_elim++;

            adaugaCampionat(tip,nr_partide,punctaj,participanti);
            char msg[BUFFER_SIZE];
            sprintf(msg, "Campionatul a fost inregistrat de %s!\n", username);
            send(client_socket , msg, strlen(msg) , 0);
        }
         else if (logged_administrator && strncmp(buffer, "vizualizare scor ", 17) == 0) 
        {
           vector<string> rez = vizualizarePunctaje();
            for (string& sir : rez)
                send(client_socket, sir.c_str(), sir.size(), 0);
        }

        /// Altele cum ar fi comanda scrisa prost sau daca nu esti logat in sistem
        else 
        {
            char error_msg[] = "Comanda necunoscuta.\n";
            send(client_socket, error_msg, strlen(error_msg), 0);
        }
    }
    close(client_socket); 
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Crearea socketului
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) 
    {
        perror("Eroare la crearea socketului");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        perror("Eroare la setarea optiunii SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("Eroare la legarea socketului");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) 
    {
        perror("Eroare la ascultarea conexiunilor");
        exit(EXIT_FAILURE);
    }

    printf("Serverul asculta pe portul %d...\n", PORT);

    while (1) 
    {
        addr_size = sizeof(client_addr);

        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) 
        {
            perror("Eroare la acceptarea conexiunii");
            exit(EXIT_FAILURE);
        }

        printf("Conexiune acceptata!\n");
        clienti_activi++;

        pid_t pid = fork();
        if (pid == -1) 
        {
            perror("Eroare la fork!");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        if (pid == 0) 
        { // Proces copil
            close(server_socket); 
            client(client_socket); 
            exit(EXIT_SUCCESS); 
        } 
        else 
        {   //Proces parinte
            close(client_socket); 
        }
    }
    return 0;
}