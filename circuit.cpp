#include <fstream>
#include "circuit.h"
#include <string>

using namespace std;
///
/// As strings que definem os tipos de porta
///

// Funcao auxiliar que testa se uma string com nome de porta eh valida
// Caso necessario, converte os caracteres da string para maiusculas
bool validType(std::string& Tipo)
{
  if (Tipo.size()!=2) return false;
  Tipo.at(0) = toupper(Tipo.at(0));
  Tipo.at(1) = toupper(Tipo.at(1));
  if (Tipo=="NT" || Tipo=="AN" || Tipo=="NA" ||
      Tipo=="OR" || Tipo=="NO" ||
      Tipo=="XO" || Tipo=="NX") return true;
  return false;
}

// Funcao auxiliar que retorna um ponteiro que aponta para uma porta alocada dinamicamente
// O tipo da porta alocada depende do parametro string de entrada (AN, OR, etc.)
// Caso o tipo nao seja nenhum dos validos, retorna nullptr
// Pode ser utilizadas nas funcoes: Circuit::setPort, Circuit::digitar e Circuit::ler
ptr_Port allocPort(std::string& Tipo)
{
  if (!validType(Tipo)) return nullptr;

  if (Tipo=="NT") return new Port_NOT;
  if (Tipo=="AN") return new Port_AND;
  if (Tipo=="NA") return new Port_NAND;
  if (Tipo=="OR") return new Port_OR;
  if (Tipo=="NO") return new Port_NOR;
  if (Tipo=="XO") return new Port_XOR;
  if (Tipo=="NX") return new Port_NXOR;

  // Nunca deve chegar aqui...
  return nullptr;
}

///
/// CLASSE CIRCUIT
///

/// ***********************
/// Inicializacao e finalizacao
/// ***********************

Circuit::Circuit(): Nin(),id_out(),out_circ(),ports() {}

Circuit::Circuit(const Circuit& C){

    Nin = C.Nin;

    for(unsigned i=0; i<C.id_out.size(); i++) id_out.push_back(C.id_out[i]);

    for(unsigned i=0; i<C.out_circ.size(); i++) out_circ.push_back(C.out_circ[i]);

    for(unsigned i=0; i<C.ports.size(); i++) ports.push_back(C.ports[i]->clone());
}

void Circuit::clear()
{
    Nin = 0;

    out_circ.clear();

    id_out.clear();

    for(unsigned i=0; i<ports.size(); i++) if(ports[i]!=nullptr) delete ports[i];
    ports.clear();
}

Circuit::~Circuit(){clear();}

void Circuit::operator=(const Circuit &C) {
    if (this!=&C){
        clear();

        Nin = C.Nin;

        for(unsigned i=0; i<C.out_circ.size(); i++) out_circ.push_back(C.out_circ[i]);

        for(unsigned i=0; i<C.id_out.size(); i++) id_out.push_back(C.id_out[i]);

        for(unsigned i=0; i<C.ports.size(); i++) ports.push_back(C.ports[i]->clone());
    }
}

void Circuit::resize(unsigned NI, unsigned NO, unsigned NP){
    if(NI>0 && NO>0 && NP>0){

        clear();
        Nin = NI;
        out_circ.resize(NO);
        id_out.resize(NO);
        ports.resize(NP);
        for(unsigned i=0; i<NO; i++)
        {
            out_circ[i] = bool3S::UNDEF;
        }

        for(unsigned i=0; i<NO; i++)
        {
            id_out[i] = 0;
        }

        for(unsigned i=0; i<NP; i++)
        {
            ports[i] = nullptr;
        }
    }
}
/// ***********************
/// Funcoes de testagem
/// ***********************

// Retorna true se IdInput eh uma id de entrada do circuito valida (entre -1 e -NInput)
bool Circuit::validIdInput(int IdInput) const
{
  return (IdInput<=-1 && IdInput>=-int(getNumInputs()));
}

// Retorna true se IdOutput eh uma id de saida do circuito valida (entre 1 e NOutput)
bool Circuit::validIdOutput(int IdOutput) const
{
  return (IdOutput>=1 && IdOutput<=int(getNumOutputs()));
}

// Retorna true se IdPort eh uma id de porta do circuito valida (entre 1 e NPort)
bool Circuit::validIdPort(int IdPort) const
{
  return (IdPort>=1 && IdPort<=int(getNumPorts()));
}

// Retorna true se IdOrig eh uma id valida para a origem do sinal de uma entrada de porta ou
// para a origem de uma saida do circuito (podem vir de uma entrada do circuito ou de uma porta)
// validIdOrig == validIdInput OR validIdPort
bool Circuit::validIdOrig(int IdOrig) const
{
  return validIdInput(IdOrig) || validIdPort(IdOrig);
}

// Retorna true se IdPort eh uma id de porta valida (validIdPort) e
// a porta estah definida (estah alocada, ou seja, != nullptr)
bool Circuit::definedPort(int IdPort) const
{
  if (!validIdPort(IdPort)) return false;
  if (ports.at(IdPort-1)==nullptr) return false;
  return true;
}

// Retorna true se IdPort eh uma porta existente (definedPort) e
// todas as entradas da porta com Id de origem valida (usa getId_inPort e validIdOrig)
bool Circuit::validPort(int IdPort) const
{
  if (!definedPort(IdPort)) return false;
  for (unsigned j=0; j<getNumInputsPort(IdPort); j++)
  {
    if (!validIdOrig(getId_inPort(IdPort,j))) return false;
  }
  return true;
}

// Retorna true se o circuito eh valido (estah com todos os dados corretos):
// - numero de entradas, saidas e portas valido (> 0)
// - todas as portas validas (usa validPort)
// - todas as saidas com Id de origem validas (usa getIdOutput e validIdOrig)
// Essa funcao deve ser usada antes de salvar ou simular um circuito
bool Circuit::valid() const
{
  if (getNumInputs()==0) return false;
  if (getNumOutputs()==0) return false;
  if (getNumPorts()==0) return false;
  for (unsigned i=0; i<getNumPorts(); i++)
  {
    if (!validPort(i+1)) return false;
  }
  for (unsigned i=0; i<getNumOutputs(); i++)
  {
    if (!validIdOrig(getIdOutput(i+1))) return false;
  }
  return true;
}

/// ***********************
/// Funcoes de consulta
/// ***********************

  unsigned Circuit::getNumInputs() const {return Nin;}
  unsigned Circuit::getNumOutputs() const {return id_out.size();}
  unsigned Circuit::getNumPorts() const {return ports.size();}

int Circuit::getIdOutput(int IdOutput) const{
    if(validIdOutput(IdOutput)){
        return id_out[IdOutput - 1];
    }
    else{
        return 0;
    }
}

bool3S Circuit::getOutput(int IdOutput) const{
    if(validIdOutput(IdOutput)){
        return out_circ[IdOutput-1];
    }
    else{
        return bool3S::UNDEF;
    }
}

std::string Circuit::getNamePort(int IdPort) const{
    if(definedPort(IdPort)){
        return ports[IdPort-1]->getName();
    }
    else{
        return "??";
    }
}

unsigned Circuit::getNumInputsPort(int IdPort) const{
    if(definedPort(IdPort)){
        return ports[IdPort-1]->getNumInputs();
    }
    else{
        return 0;
    }
}

int Circuit::getId_inPort(int IdPort, unsigned I) const{
    if(definedPort(IdPort) && I>=0 && I<getNumInputsPort(IdPort)){
        return ports[IdPort-1]->getId_in(I);
    }
    else{
        return 0;
    }
}
/// ***********************
/// Funcoes de modificacao
/// ***********************

void Circuit::setIdOutput(int IdOut, int IdOrig){
    if(validIdOutput(IdOut) && validIdOrig(IdOrig))
    {
        id_out[IdOut-1] = IdOrig;
    }
}

void Circuit::setPort(int IdPort, std::string Tipo, unsigned NIn){

    if(!validIdPort(IdPort)) return;
    if(!validType(Tipo)) return;
    if(Tipo=="NT") { if(NIn!=1) return;}
    else
    {
        if(NIn<2 || NIn>4) return;
    }
    if(ports[IdPort-1]!=nullptr) delete ports[IdPort-1];
    ports[IdPort-1] = allocPort(Tipo);
    ports[IdPort-1]->setNumInputs(NIn);
}

void Circuit::setId_inPort(int IdPort, unsigned I, int IdOrig) const{
    if(!definedPort(IdPort)) return;
    if(!ports[IdPort-1]->validIndex(I)) return;
    if(!validIdOrig(IdOrig)) return;
    ports[IdPort-1]->setId_in(I,IdOrig);
}

/// ***********************
/// E/S de dados
/// ***********************

void Circuit::digitar(){

    unsigned NI, NO, NP;
    std::string tipo;
    do
    {
     std::cout<<"Digite a quantidade de entradas :";
     std::cin>>NI;
     cout<<"Digite a quandidade de saidas: ";
     cin>>NO;
     cout<<"Digite a quantidade de portas do circuito: ";
     cin>>NP;
     if (NI<=0 || NO<=0 || NP<=0) cerr<< "Voce digitou algum dado invalido(<=0), digite novamente:\n";
    } while(NI<=0 || NO<=0 || NP<=0);

    resize(NI, NO, NP);
    for(unsigned i=0; i<getNumPorts();i++)
    {
        do
        {
            cout<<"Digite o tipo da porta (NT,AN,NA,OR,NO,XO,NX): ";
            cin>>tipo;
            if(!validType(tipo)) cerr<<"Voce digitou um tipo de porta invalido, digite novamente:\n";
        }while(!validType(tipo));
        ports[i] = allocPort(tipo);
        ports[i]->digitar();
    }
    cout<<"Digite as ids das portas de saida: "<<endl;
    for(unsigned i=0;i<getNumOutputs();i++)
    {

        do
        {
            cout<<"Id da saida "<<i<<": ";
            cin>>id_out[i];
            if(!validIdOrig(id_out[i])) cout<<"Voce digitou uma id invalida, digite novamente:\n";

        }while(!validIdOrig(id_out[i]));
    }
}

bool Circuit::ler(const string& arq){

        ifstream arq_ler(arq.c_str());

        if (!arq_ler.is_open()) {return false;}


        string circ, port, tipo, saidas, lixo, dpt;
        unsigned NI, NO, NP, numInPort, indice_saida;
        int Id;

        arq_ler>>circ;
        if(circ=="CIRCUITO"){
            arq_ler>>dpt;
            if(dpt!=":"){
                arq_ler.close();
                return false;
            }
        }
         else if(circ!="CIRCUITO:")
        {
            arq_ler.close();
            return false;
        }

        arq_ler>>NI>>NO>>NP;

       if((NI<=0) || (NO<=0) || (NP<=0))
        {
            arq_ler.close();
            return false;
        }

        clear();
        resize(NI, NO, NP);

        arq_ler>>port;
        if(port=="PORTAS"){
            arq_ler>>dpt;
            if(dpt!=":"){
                clear();
                arq_ler.close();
                return false;
            }
        }
        else if(port!="PORTAS:")
        {
            clear();
            arq_ler.close();
            return false;
        }

        for(unsigned i=0; i<getNumPorts();i++)
        {
            arq_ler>>Id>>lixo>>tipo>>numInPort>>lixo;

            if(Id!=i+1){
                clear();
                arq_ler.close();
                return false;
            }
            if(tipo=="NT" && numInPort!=1)
            {
                clear();
                arq_ler.close();
                return false;
            } else if((!validIdPort(Id))|| (!validType(tipo)) || ((numInPort<2 || numInPort>4)&& tipo!="NT"))
            {
                clear();
                arq_ler.close();
                return false;
            }
            ports[i] = allocPort(tipo);

            ports[i]->setNumInputs(numInPort);

            for(unsigned j=0; j<ports[i]->getNumInputs(); j++)
            {
                arq_ler>>Id;
                if(!validIdOrig(Id)){
                    clear();
                    arq_ler.close();
                    return false;
                }
                ports[i]->setId_in(j,Id);
            }
        }

        arq_ler>>saidas;
        if(saidas=="SAIDAS"){
            arq_ler>>dpt;
            if(dpt!=":"){
                clear();
                arq_ler.close();
                return false;
            }
        }
        else if(saidas!="SAIDAS:")
        {
            clear();
            arq_ler.close();
            return false;
        }
        for(unsigned i=0; i<getNumOutputs();i++)
        {
            arq_ler>>Id>>lixo>>indice_saida;

            if(Id!=i+1){
                clear();
                arq_ler.close();
                return false;
            }
            if(!validIdOutput(Id) || !validIdOrig(indice_saida))
            {
                clear();
                arq_ler.close();
                return false;
            }
            setIdOutput(Id, indice_saida);
        }

        return true;
}

ostream& Circuit::imprimir(ostream& O) const{
    if(!valid()) return O;

    O<<"CIRCUITO: "<<getNumInputs()<<" "<<getNumOutputs()<<" "<<getNumPorts()<<endl;

    O<<"PORTAS:"<<endl;
    for(unsigned i=0; i<getNumPorts(); i++)
    {
        O<<i+1<<") "<<getNamePort(int(i+1))<<" "<<getNumInputsPort(int(i+1))<<": ";
        for(unsigned j=0; j<getNumInputsPort(int(i+1)); j++){O<<getId_inPort(i+1, j)<<" ";}
        O<<endl;
    }
    O<<"SAIDAS:"<<endl;
    for(unsigned i=0; i<getNumOutputs(); i++)
    {
        O<<i+1<<")"<<" "<<getIdOutput(int(i+1))<<endl;
    }
    return O;

}

bool Circuit::salvar(const std::string& arq) const{

    ofstream arq_salvar(arq.c_str());
    if (!arq_salvar.is_open()) return false;
    imprimir(arq_salvar);
    return true;
}

/// ***********************
/// SIMULACAO (funcao principal do circuito)
/// ***********************

bool Circuit::simular(const std::vector<bool3S>& in_circ){

    if(!valid()) return false;

     bool tudo_def, alguma_def;
     vector<bool3S> in_port(4);
     int id;

     for (unsigned i = 0;i <getNumPorts(); i++){
        ports[i]->setOutput(bool3S::UNDEF);
     }
     do{
        tudo_def = true;
        alguma_def = false;

        for (unsigned i = 0;i <getNumPorts(); i++){

            if(ports[i]->getOutput() == bool3S::UNDEF){

                for (unsigned j = 0; j<ports[i]->getNumInputs(); j++){

                    id = ports[i]->getId_in(j);

                    if(id>0){
                        in_port[j] = ports[id-1]->getOutput();
                    }
                    else{
                        in_port[j] = in_circ[-id-1];
                    }
                }

                ports[i]->simular(in_port);

                if(ports[i]->getOutput() == bool3S::UNDEF){
                    tudo_def = false;
                }
                else{
                    alguma_def = true;
                }
            }

        }

    }while(!tudo_def && alguma_def);

     for (unsigned j = 0; j < getNumOutputs(); j++){

        id = id_out[j];

        if(id>0){
            out_circ[j] = ports[id-1]->getOutput();
        }
        else{
            out_circ[j] = in_circ[-id-1];
        }
    }
    return true;
}
