#include <iostream>
#include <string.h>
#include <fstream>

using namespace std;

struct Pokemon {
  unsigned id;
  char nome[15]; // utilizar este campo como chave
  char tipo[15];
  unsigned total;
  unsigned ataque;
  unsigned defesa;
  unsigned at_esp;
  unsigned def_esp;
  unsigned velocidade;
};


typedef Pokemon dado; // a arvore guarda informacoes do tipo Dado
typedef unsigned tipoChave; // tipo da chave usada na comparação

// capacidade máxima e mínima do pacote
const unsigned CAP_PACOTE = 9;
const unsigned MIN_PACOTE = 4;

// posição do elemento do meio do pacote
const unsigned POS_MEIO = 4;

// posição inválida no disco
const int POS_INVALIDA = -1;

ostream& operator<<(ostream& saida, const Pokemon& Vet) {
	saida << "(" << Vet.id << "," << Vet.nome << "," << Vet.tipo << "," << Vet.total << "," << Vet.ataque <<","<< Vet.defesa <<
			 "," << Vet.at_esp << "," << Vet.def_esp << "," << Vet.velocidade << "," << ")";
	return saida;
}

istream& operator>>(istream& entrada, Pokemon& Vet) {
	entrada >> Vet.id >> Vet.nome >> Vet.tipo >> Vet.total >> Vet.ataque >> Vet.defesa >> 
			   Vet.at_esp >> Vet.def_esp >> Vet.velocidade;
	return entrada;
}

// essa classe também poderia se chamar bloco, pagina, packet ou pedaco
class pacote {
	friend class ArvoreBMais;
    friend class sequenceset;
    // classe não possui métodos públicos
    // objetos da classe são criados e manuseados
    // apenas pela classe sequenceset
    private:
        dado elementos[CAP_PACOTE];
        unsigned numElementos;
        int posProximoPacote;
        pacote(): numElementos(0), posProximoPacote(POS_INVALIDA) { }
        bool cheio() { return (numElementos == CAP_PACOTE); }
        void inserir(dado umDado);
        inline bool chaveEhMaiorQueTodos(tipoChave chave);
        inline bool chaveEhMenorQueTodos(tipoChave chave);
        void imprimir();
};

void pacote::inserir(dado umDado) {
    int posicao = numElementos - 1;
    // Faz a procura pela posição de inserção do elemento de forma decrescente
    while ( (posicao >= 0) and umDado.id < elementos[posicao].id) {
        elementos[posicao+1] = elementos[posicao];
        posicao--;
    }
    elementos[posicao+1] = umDado;
    numElementos++;
}

void pacote::imprimir() {
    cout << "[";
    for (unsigned i = 0; i < numElementos; i++)
        cout << "(" << elementos[i].id << "/" << elementos[i].total << ")";
    cout << "]";
}

bool pacote::chaveEhMaiorQueTodos(tipoChave chave) {
    return ( elementos[numElementos-1].id < chave );
}

bool pacote::chaveEhMenorQueTodos(tipoChave chave) {
    return ( elementos[0].id > chave );
}

// Cabeçalho do Arquivo do Sequence Set
struct cabecalhoArqSS {
	unsigned capacidadeMaxPacote; 
	unsigned capacidadeMinPacote;
	unsigned posicaoMeio;  
	unsigned numPacotes;
	int posPrimeiroPacote;  
	int proxPosicaoVazia;   // para poder reaproveitar pacotes que 
							// fiquem vazios no meio do arquivo
};

class sequenceset {
	private:
		unsigned numPacotes;
		int posPrimeiroPacote;
		int proxPosicaoVazia; 
		string nomeArquivo; 
		inline bool cabecalhoEhConsistente(const cabecalhoArqSS& umCabecalho);
		void atualizarCabecalhoNoArquivo();
		void gravarPacoteNoArquivo(pacote* umPacote, unsigned posicao);
		void lerPacoteDoArquivo(pacote* umPacote, unsigned posicao);
		unsigned encontrarProxPosDisponivel();
		unsigned encontrarPacoteParaInsercao(pacote* umPacote, dado umDado);
		pacote* dividirPacote(pacote* umPacote, unsigned posNovoPacote); 
		dado buscaBinaria(dado vetor[], int inicio, int fim, tipoChave chave);       
	public:
		sequenceset(string arquivo);
		~sequenceset();
		void inserirDado(dado umDado);
		void imprimir();
		void depurar();
		dado buscar(tipoChave chave);
		/* 
		void remover(tipoChave chave);
		*/
};

sequenceset::sequenceset(string arquivo) {
	nomeArquivo = arquivo;
	ifstream arqEntrada(nomeArquivo);
	cabecalhoArqSS cabecalho;

	// verifica se o arquivo existe, se sim, lê o cabeçalho 
	// e verifica se os dados são consistentes com a configuração
	// do sequence set
	if ( arqEntrada ) {
		arqEntrada.read((char*) &cabecalho, sizeof(cabecalhoArqSS));
		arqEntrada.close();
		if (not cabecalhoEhConsistente(cabecalho)) {
			throw runtime_error("Aplicação usa configuração diferente das usadas no arquivo");
		}
		// atualiza os dados do sequence set de acordo com o cabeçalho do arquivo
		numPacotes = cabecalho.numPacotes;
		posPrimeiroPacote = cabecalho.posPrimeiroPacote;
		proxPosicaoVazia = cabecalho.proxPosicaoVazia;
	} 
	// não existe o arquivo de entrada ainda, inicializa novo sequence set
	// e salva o cabeçalho no arquivo
	else { 
		numPacotes = 0;
		posPrimeiroPacote = POS_INVALIDA;
		proxPosicaoVazia = POS_INVALIDA;
		// cria o arquivo
		ofstream ArqSaida(nomeArquivo);
		ArqSaida.close();
		atualizarCabecalhoNoArquivo();
	}    
}

sequenceset::~sequenceset() {
	// apenas atualiza o cabeçalho, para garantir que esteja tudo ok
	atualizarCabecalhoNoArquivo(); 
}

bool sequenceset::cabecalhoEhConsistente(const cabecalhoArqSS& umCabecalho) {
	return ((umCabecalho.capacidadeMaxPacote == CAP_PACOTE)
			 and (umCabecalho.capacidadeMinPacote == MIN_PACOTE)
			 and (umCabecalho.posicaoMeio == POS_MEIO));    
}

void sequenceset::atualizarCabecalhoNoArquivo() {
	cabecalhoArqSS cabecalho; 
	cabecalho.capacidadeMaxPacote = CAP_PACOTE;
	cabecalho.capacidadeMinPacote = MIN_PACOTE;
	cabecalho.posicaoMeio = POS_MEIO;
	cabecalho.numPacotes = numPacotes;
	cabecalho.posPrimeiroPacote = posPrimeiroPacote;
	cabecalho.proxPosicaoVazia = proxPosicaoVazia;
	// precisa ser fstream para não apagar o arquivo já existente
	fstream arqSaida(nomeArquivo, ios::in | ios::out | ios::binary);
	arqSaida.write((const char*) &cabecalho, sizeof(cabecalhoArqSS));
	arqSaida.close();    
}

void sequenceset::lerPacoteDoArquivo(pacote* umPacote, unsigned posicao) {
	 // pula o cabeçalho do arquivo e o número de páginas anteriores
	unsigned posArq = sizeof(cabecalhoArqSS) + posicao*sizeof(pacote);  
	fstream arqEntrada(nomeArquivo, ios::in | ios::out | ios::binary);
	arqEntrada.seekg(posArq);
	arqEntrada.read((char*) umPacote, sizeof(pacote));
	arqEntrada.close();    
}


void sequenceset::gravarPacoteNoArquivo(pacote* umPacote, unsigned posicao) {
	 // pula o cabeçalho do arquivo e o número de páginas anteriores
	unsigned posArq = sizeof(cabecalhoArqSS) + posicao*sizeof(pacote); 
	// precisa ser fstream para não apagar o arquivo já existente
	fstream arqSaida(nomeArquivo, ios::in | ios::out | ios::binary);
	arqSaida.seekp(posArq);
	arqSaida.write((const char*) umPacote, sizeof(pacote));
	arqSaida.close();   
}

unsigned sequenceset::encontrarProxPosDisponivel() {
	// se não tem pacote vazio no meio do arquivo
	// então a próxima posição disponível será uma posição a mais
	// que a última do arquivo (numPacotes-1)
	if (proxPosicaoVazia == POS_INVALIDA) {
		return numPacotes;  
	} else {
		// este trecho código só é útil com remoção
		// como ainda não temos remoção, então gera exceção
		throw runtime_error("Não era para proxPosicaoVazia ser diferente de POS_INVALIDA");

		// reaproveitar primeira posição vazia
		unsigned posicao = proxPosicaoVazia;
		pacote* pacoteInvalido = new pacote();
		lerPacoteDoArquivo(pacoteInvalido, proxPosicaoVazia);
		proxPosicaoVazia = pacoteInvalido->posProximoPacote;
		return posicao; 
	}
}


void sequenceset::inserirDado(dado umDado) {
	unsigned posicao;
	pacote* pacoteDestino = new pacote();
	posicao = encontrarPacoteParaInsercao(pacoteDestino, umDado);
	// pacote está cheio, precisa dividir
	if ( pacoteDestino->cheio() ) {
		unsigned posicaoNovoPacote = encontrarProxPosDisponivel();
		pacote* novoPacote = dividirPacote(pacoteDestino, posicaoNovoPacote); 
		if ( umDado.id > novoPacote->elementos[0].id )
			novoPacote->inserir(umDado);
		else 
			pacoteDestino->inserir(umDado);
		gravarPacoteNoArquivo(pacoteDestino, posicao);
		gravarPacoteNoArquivo(novoPacote, posicaoNovoPacote);
		delete novoPacote;
		numPacotes++;
		atualizarCabecalhoNoArquivo();
	}
	else {
		pacoteDestino->inserir(umDado);
		gravarPacoteNoArquivo(pacoteDestino, posicao);
	}
	delete pacoteDestino;
}

// o método encontrarPacoteParaInsercao retorna dois elementos:
// um pacote, já carregado do disco, em que será feita a inserção
// e a posição relativa desse pacote no disco, para facilitar
// a gravação posterior, após alterações
// o método recebe um pacote recém-criado e o dado para busca
unsigned sequenceset::encontrarPacoteParaInsercao(pacote* umPacote, dado umDado) {
	// caso vetor expansível esteja vazio, criar primeiro pacote
	if (posPrimeiroPacote == POS_INVALIDA) {
		posPrimeiroPacote = encontrarProxPosDisponivel();
		numPacotes = 1;
		atualizarCabecalhoNoArquivo();
		return posPrimeiroPacote;
	}
	// sequence set não está vazio: procura o pacote para inserir o elemento
	else {
		lerPacoteDoArquivo(umPacote, posPrimeiroPacote);
		unsigned posicao = posPrimeiroPacote;

		// este laço vai lendo pacotes do disco, enquanto a chave
		// for maior que os valores do pacote atual
		while ( (umPacote->posProximoPacote != POS_INVALIDA)
				 and (umPacote->chaveEhMaiorQueTodos(umDado.id)) ) {
			posicao = umPacote->posProximoPacote;
			lerPacoteDoArquivo(umPacote, posicao);
		}
		return posicao;
	}
}



pacote* sequenceset::dividirPacote(pacote* umPacote, unsigned posNovoPacote) {
	pacote* novo = new pacote();
	// copia metade superior dos dados do pacote atual para o novo
	for (unsigned i = 0; i <= CAP_PACOTE/2; i++) {
		novo->elementos[i] = umPacote->elementos[i + CAP_PACOTE/2];
	}
	novo->posProximoPacote = umPacote->posProximoPacote;
	umPacote->posProximoPacote = posNovoPacote;
	novo->numElementos = CAP_PACOTE - CAP_PACOTE/2;
	umPacote->numElementos = CAP_PACOTE/2; 
	return novo;
}

dado sequenceset::buscar(tipoChave chave) {
	if (posPrimeiroPacote == POS_INVALIDA) {
		throw runtime_error("Busca: Sequence Set vazio.");
	}
	// sequence set não está vazio: 
	// procura o pacote que poderia conter o elemento
	else {
		pacote* umPacote = new pacote();
		lerPacoteDoArquivo(umPacote, posPrimeiroPacote);

		// este laço vai lendo pacotes do disco, enquanto a chave
		// for maior que os valores do pacote atual
		while ( (umPacote->posProximoPacote != POS_INVALIDA)
				 and (umPacote->chaveEhMaiorQueTodos(chave)) ) {
			lerPacoteDoArquivo(umPacote, umPacote->posProximoPacote);
		}
		// ou o dado está no pacote que saiu do while 
		// ou não existe no sequence set - precisa agora buscar o elemento no 
		// vetor de elementos

		return buscaBinaria(umPacote->elementos, 0, umPacote->numElementos-1, chave);
	} 
}

//funcao de busca binaria recursiva
dado sequenceset::buscaBinaria(dado vetor[], int inicio, int fim, tipoChave chave) {
	int meio = (inicio+fim)/2;

	if (inicio <= fim) {
		if (chave > vetor[meio].id)
			return buscaBinaria(vetor,meio+1,fim,chave);
		else if (chave < vetor[meio].id)
			return buscaBinaria(vetor,inicio,meio-1,chave);
		else
			return vetor[meio];
	} else { // inicio == fim, ou seja, não há mais o que buscar
		// retornamos -1 para indicar posição não encontrada
		throw runtime_error("Busca: elemento não encontrado.");
	} 
}


void sequenceset::imprimir() {
	if (numPacotes > 0) {
		pacote* auxiliar = new pacote();
		lerPacoteDoArquivo(auxiliar,posPrimeiroPacote);
		
		while (auxiliar->posProximoPacote != POS_INVALIDA) {
			auxiliar->imprimir();
			cout << "->";
			lerPacoteDoArquivo(auxiliar,auxiliar->posProximoPacote);
		}
		auxiliar->imprimir();
		delete auxiliar;
	}
	cout << endl;  
}

void sequenceset::depurar() {
	cout << "-- Dados do Sequence Set --" << endl
		 << "Número de pacotes: " << numPacotes << endl
		 << "Posição do primeiro pacote: " << posPrimeiroPacote << endl
		 << "Próxima posição vazia (-1 se não tiver remoção): " << proxPosicaoVazia << endl;
	if (numPacotes > 0) {
		cout << "Dados dos pacotes no formato: "
			 << "{posição do pacote}(número de elementos/próximo pacote)[elementos]"
			 << endl;
		pacote* auxiliar = new pacote();
		unsigned posicao = posPrimeiroPacote;
		lerPacoteDoArquivo(auxiliar,posPrimeiroPacote);
		
		while (auxiliar->posProximoPacote != POS_INVALIDA) {
			cout << "{" << posicao << "}" 
				 << "(" << auxiliar->numElementos << "/" 
				 << auxiliar->posProximoPacote << ")";
			auxiliar->imprimir();
			cout << "->";
			posicao = auxiliar->posProximoPacote;
			lerPacoteDoArquivo(auxiliar,auxiliar->posProximoPacote);
		}
		cout << "(" << auxiliar->numElementos << "/" 
			 << auxiliar->posProximoPacote << ")";
		auxiliar->imprimir();
		delete auxiliar;
	}
	cout << endl << "-- Fim dos Dados --" << endl;  
}

// constantes para verifir se esta certo as paginas
const unsigned MAXIMODEDADOS = 9; // maximo de dados por pagina
const unsigned MINIMODEDADOS = 4; // minimo de dados por pagina
const unsigned MAXIMODEFILHOS = 10; // maximo de filhos 
const unsigned MEIODOSDADOS = 4; // para subir para pagina pai em caso de estouro

typedef Pokemon Dado;

class Noh{
	friend class ArvoreBMais;
	private:
		bool folha;
		unsigned num;
		Pokemon vetorDados[MAXIMODEDADOS]; // Para Guardar os Dados da struct tipo Pokemon
		Noh* filhos[MAXIMODEFILHOS];
		unsigned vetorDeDados[MAXIMODEFILHOS];
		
	public:
		Noh();
		~Noh();
};

Noh :: Noh(){
	num = 0;
	folha = false;
	for (unsigned i = 0; i < MAXIMODEFILHOS; i++){
		filhos[i] = nullptr;
	}
	for (unsigned i = 0; i < MAXIMODEFILHOS; i++){
		vetorDeDados[i] = 0;
	}
}
	
Noh :: ~Noh(){
	for (unsigned i = 0; i < num + 1; i++){
		delete filhos[i];
	}
}

class ArvoreBMais{
	private:
		string umArquivo; // arquivo de dados
		friend ostream& operator<<(ostream& output, ArvoreBMais& arvore);
		Noh* Raiz;
		void percorreEmOrdemAux(Noh* atual , int nivel);
		// busca recursiva
		Dado buscaAux(Noh* RaizSub , unsigned chave);
		// funcoes auxiliares para insercao de um Dado d no Noh umNoh
		Noh* insereAux(Noh* umNoh , const Pokemon& umDado , Pokemon& umDadoPromovido);
		Noh* divideNoh(Noh* umNoh , const Pokemon& umDado , Pokemon& umDadoPromovido);
		void insereEmNohFolhaNaoCheio(Noh* umNoh , Pokemon umDado);
		void insereEmNohIntermediarioNaoCheio(Noh* umNoh , Noh* novoNoh , Pokemon& umDadoPromovido);
		void imprimirAux(Noh* RaizSub , int nivel);
		void atualizaEmOrdemAux(Noh* atual , int& nivel);
	public:
		ArvoreBMais(string arquivoE);
		~ArvoreBMais();
		void insere(const Pokemon& umDado);
		// Busca um item na Arvore a partur da chave
		void Busca(unsigned chave);
		void percorreEmOrdem();
		void imprimir();
		void atualizaEmOrdem();
		Dado buscaEmArquivo(unsigned Pos , unsigned umaChave);
};

ArvoreBMais :: ArvoreBMais(string arquivoE){
	Raiz = nullptr;
	umArquivo = arquivoE;
}

ArvoreBMais :: ~ArvoreBMais(){
	delete Raiz;
}

Dado ArvoreBMais :: buscaEmArquivo(unsigned Pos , unsigned umaChave){
	unsigned posicaoDoArquivo;
	pacote Pacote;
	unsigned i = 0;
	
	fstream arquivoDeEntrada("teste.dat" , ios::in | ios::out | ios::binary);
	posicaoDoArquivo = sizeof(cabecalhoArqSS) + Pos * sizeof(pacote);
	arquivoDeEntrada.seekg(posicaoDoArquivo);
	arquivoDeEntrada.read((char*) &Pacote , sizeof(pacote));
	arquivoDeEntrada.close();
	
	while(i < Pacote.numElementos and Pacote.elementos[i].id <= umaChave){
		i++;
	}
	
	i--;
	
	if(Pacote.elementos[i].id == umaChave){
		return Pacote.elementos[i];
	}else{
		Dado naoAchou;
		naoAchou.id = -1;
		return naoAchou;
	}
} 

void ArvoreBMais :: insere(const Pokemon& umDado){
	// se a arvore tiver vazia , aloca um noh folha para a raiz
	// insere na raiz
	if(Raiz == nullptr){
		Raiz = new Noh;
		Raiz->folha = true;
		Raiz->vetorDados[0] = umDado;
		Raiz->num = 1;
	}else{
		// ja tem algo na raiz
		// para prencher item de filho que foi dividido
		Dado umDadoPromovido;
		Noh* novoNoh; // se gero divisao e criado um novo Noh
		// chama metodo auxiliar recursivo
		novoNoh = insereAux(Raiz , umDado , umDadoPromovido);
		// Verifica se houve divisao na raiz
		if(novoNoh){
			// se novoNoh nao e nulo ,entao houve divisao
			// cria nova raiz apontando para cima da antiga raiz e novoNoh com filhos
			Noh* antigaRaiz = Raiz;
			Raiz = new Noh;
			Raiz->vetorDados[0] = umDadoPromovido;
			Raiz->num = 1;
			Raiz->filhos[0] = antigaRaiz;
			Raiz->filhos[1] = novoNoh;
		}
	}
}

Noh* ArvoreBMais :: insereAux(Noh* umNoh ,const Pokemon& umDado , Dado& umDadoPromovido){
	// Caso umNoh seja Folha , ajeita o local para inserir novo dado
	if(umNoh->folha){
		//Verifica se o umNoh nao esta cheio
		if(umNoh->num < MAXIMODEDADOS){
			// nao esta cheio , basta inserir
			insereEmNohFolhaNaoCheio(umNoh , umDado);
			return nullptr;
		}else{
			// Se o umNoh esta cheio , precisa dividir
			Noh* novoNoh;
			novoNoh = divideNoh(umNoh , umDado , umDadoPromovido);
			// verifica quem vai receber o item , se umNoh ou novoNoh
			if(umDado.id <= novoNoh->vetorDados[0].id){
				insereEmNohFolhaNaoCheio(umNoh , umDado);
			}else{
				insereEmNohFolhaNaoCheio(novoNoh , umDado);
			}
			return novoNoh;
		}
	}else{
		// umNoh nao e folha
		// encontra um filho que ira receber novo item
		int i = umNoh->num-1;
		
		while(i >= 0 and umNoh->vetorDados[i].id > umDado.id){
			--i;
		}
		Noh* nohAux;
		nohAux = insereAux(umNoh->filhos[i+1] , umDado , umDadoPromovido);
		// verifica se nao houve estouro do filho
		if(nohAux){
			// se novoNoh nao e nulo , houve divisao
			// verifica se nao deve dividir o noh atual
			// para isso armazena umDadoPromovido em variavel auxiliar
			Dado itemAux = umDadoPromovido;
			if(umNoh->num < MAXIMODEDADOS){
				insereEmNohIntermediarioNaoCheio(umNoh , nohAux , itemAux); // umNoh nao esta cheio , so arrumar o estouro do filho)
				return nullptr;
			}else{
				Noh* novoNoh; // umNOh esta cheio , divide antes de arrumar o estouro do filho
				novoNoh = divideNoh(umNoh , umDado , umDadoPromovido);
				
				if(itemAux.id <= umNoh->vetorDados[MEIODOSDADOS].id){ // verifica quem vai receber novo noh e item promovido , umNoh ou novoNoh
					insereEmNohIntermediarioNaoCheio(umNoh , nohAux , itemAux); // noh e item ficam em umNoh
				}else{
					insereEmNohIntermediarioNaoCheio(novoNoh , nohAux , itemAux); // noh e item ficam em novoNoh
				}
				return novoNoh;
			}
		} // caso novoNoh seja nulo , nao faz nada
	}
	return nullptr;
}

Noh* ArvoreBMais :: divideNoh(Noh* umNoh ,const Pokemon& umDado , Pokemon& umDadoPromovido){

	Noh* novoNoh = new Noh;
	novoNoh->folha = umNoh->folha;
	
	if(not umNoh->folha){ // se o noh nao e folha , divide os filhos
		for (unsigned i = 0; i < MEIODOSDADOS + 1; i++){
			novoNoh->filhos[i] = umNoh->filhos[MEIODOSDADOS + 1 + i];
		}
	}
	
	// Faz as copias de umNoh e nao movelo pra cima
	for (unsigned i = 0; i < MEIODOSDADOS + 1; i++){
		novoNoh->vetorDados[i] = umNoh->vetorDados[MEIODOSDADOS + 1 + i];
		novoNoh->num++;
		umNoh->num--;
	}
	
	umDadoPromovido = novoNoh->vetorDados[0];
	
	return novoNoh;
}


void ArvoreBMais :: insereEmNohFolhaNaoCheio(Noh* umNoh , Pokemon umDado){
	int i = umNoh->num-1;
	
	while(i >= 0 and umNoh->vetorDados[i].id > umDado.id){
		umNoh->vetorDados[i + 1] = umNoh->vetorDados[i];
		i--;
	}
	
	umNoh->vetorDados[i + 1] = umDado; // insere novo item no local encontrado
	umNoh->num++;
}

void ArvoreBMais :: insereEmNohIntermediarioNaoCheio(Noh* umNoh , Noh* novoNoh , Pokemon& umDadoPromovido){
	int i = umNoh->num-1;
	
	while (i >= 0 and umNoh->vetorDados[i].id > umDadoPromovido.id){ // move item uma posicao a direita
		umNoh->vetorDados[i+1] = umNoh->vetorDados[i];  // move o filho a direita de item uma posicao a direita
		umNoh->filhos[i+2] = umNoh->filhos[i+1];
		i--;
	}
	
	umNoh->vetorDados[i+1] = umDadoPromovido; // insere novo item no local encontrada
	umNoh->filhos[i+2] = novoNoh; 	// insere novo noh (uma posicao a frente no vetor de filhos
	umNoh->num++;
}

void ArvoreBMais :: atualizaEmOrdem(){
	int cont = 0;
	atualizaEmOrdemAux(Raiz , cont);
	cout << endl;
}

void  ArvoreBMais :: atualizaEmOrdemAux(Noh* umNoh , int& nivel){
	unsigned i; 
	if(umNoh == nullptr){
		throw runtime_error("Erro!") ;
	}
	
	for (i = 0; i < umNoh->num; i++){
		// se noh nao e folha , imprima os dados do filho i
		// antes de imprimir o item i
		if(not umNoh->folha){
			atualizaEmOrdemAux(umNoh->filhos[i] , nivel);
		}else{
			for (unsigned j = 0; j < umNoh->num + 1; j++){
				umNoh->vetorDeDados[j] = nivel;
				nivel++;
			}
		}
	}
	
	// imprima os dados do ultimo filho
	if(not umNoh->folha){
		atualizaEmOrdemAux(umNoh->filhos[i] , nivel);
	}else{
		for (unsigned j = 0; j < umNoh->num + 1; j++){
			umNoh->vetorDeDados[j] = nivel;
			nivel++;
		}
	}
}

void ArvoreBMais :: percorreEmOrdem(){
	percorreEmOrdemAux(Raiz , 0);
	cout << endl;
}

void  ArvoreBMais :: percorreEmOrdemAux(Noh* umNoh , int nivel){
	unsigned i; 
	// caso nao tiver dados da erro
	if(umNoh == nullptr){
		throw runtime_error("Erro!") ;
	}
	// se nao for folha imprima os dados em ordem
	for (i = 0; i < umNoh->num; i++){
		if(not umNoh->folha){
			percorreEmOrdemAux(umNoh->filhos[i] , nivel + 1);
		}
		cout << umNoh->vetorDados[i].id << '/' << nivel << ' ';
	}
	
	if(not umNoh->folha){
		percorreEmOrdemAux(umNoh->filhos[i] , nivel + 1);
	}
}


void ArvoreBMais :: Busca(unsigned chave){
    Dado elementoBuscado;
    
	//se nao tiver nada na raiz , a arvore entao esta vazia
    if(Raiz == nullptr){
       throw runtime_error ("Arvore vazia!");
    }else{
		elementoBuscado = buscaAux(Raiz , chave);
		if(elementoBuscado.id < 0){ // se o achou for menor que 0 entao e um numero negativo entao nao e unsigned e o elemento nao se encontra  
			throw runtime_error ("Erro!");
		}
    }
}

Dado ArvoreBMais :: buscaAux(Noh* RaizSub , unsigned chave){
    unsigned i = 0;
    
    while((i < RaizSub->num)  and (RaizSub->vetorDados[i].id <= chave)){ //percorre RaizSub ate achar um item com chave maior ou igual a procurada
        i++;
	}
        
    i--; // returna a posicao anterior (desfazendo o ultimo incremento)

    // se e igual , entao nos achamos o elemento

    if(RaizSub->vetorDados[i].id == chave){
        return RaizSub->vetorDados[i];
    }else{
        // se noh nao e folha , enta nao tem o elemento buscado , insere no arquivo as posicoes
        if(RaizSub->folha){
			return buscaEmArquivo(RaizSub->vetorDeDados[i+1], chave);
        }
        //nao e folha , desce em noh filho a esquerda do item da chave
        //maior que a procurada
        else{
            return buscaAux(RaizSub->filhos[i+1], chave);
        }
    }
}

ostream& operator<<(ostream& output, ArvoreBMais& arvore) {
	// arvore.percorreEmOrdemAux(arvore.raiz,0);
	arvore.imprimir();
	return output;
}

// imprime arvore os dados em Ordem de forma Generica
void ArvoreBMais :: imprimir(){
	imprimirAux(Raiz, 0);
	cout << endl;

}

void ArvoreBMais :: imprimirAux(Noh* RaizSub , int nivel){
	unsigned i; 
	
	if(RaizSub == nullptr){ // se nao tiver nada na raiz entao a arvore esta vazia
		throw runtime_error("Erro!") ;
	}
	
	for (i = 0; i < RaizSub->num; i++){
		// se noh nao e folha , imprima os dados
		if(not RaizSub->folha){
				imprimirAux(RaizSub->filhos[i] , nivel + 1);
			}
			cout << endl;
			cout << "Nivel da Arvore : " <<"[ "<< nivel << " ]" << endl <<"Id : " << RaizSub->vetorDados[i].id << endl
				 << "Nome : " << RaizSub->vetorDados[i].nome << endl
				 << "Tipo : " << RaizSub->vetorDados[i].tipo << endl
				 << "Total : "<< RaizSub->vetorDados[i].total << endl 
				 << "Ataaque : " << RaizSub->vetorDados[i].ataque << endl 
				 << "Defesa : "<< RaizSub->vetorDados[i].defesa << endl
				 << "At esp : " << RaizSub->vetorDados[i].at_esp << endl 
				 << "At def : "<< RaizSub->vetorDados[i].def_esp << endl
				 << "Velocidade : " << RaizSub->vetorDados[i].velocidade 
				 << endl;
	}
	
	if(not RaizSub->folha){
		imprimirAux(RaizSub->filhos[i] , nivel+1);
		
	}
}

int main() {
	sequenceset meuSeqSet("teste.dat");
	dado umDado;
	tipoChave umaChave;
	char operacao;
	
		
	do {
		try {
			cin >> operacao;
			switch (operacao) {
				case 'i': // inserir
					cin >> umDado;
					meuSeqSet.inserirDado(umDado);
					break;
				case 'b': // buscar
					cin >> umaChave;
					umDado = meuSeqSet.buscar(umaChave);
					cout << "Busca: "<< umDado << endl;
					break;
				case 'p': // mostrar estrutura
					meuSeqSet.imprimir();
					break;
				case 'd': // mostrar estrutura
					meuSeqSet.depurar();
					break;
				case 's': // sair
					// será tratado no while
					break;
				default:
					cout << "Opção inválida!" << endl;
			}
		} catch (runtime_error& e) {
			cerr << e.what() << endl;
		}
	} while (operacao != 's');
	
	cabecalhoArqSS Cabecalho;
	
	fstream arqEntrada("teste.dat", ios::in | ios::out | ios::binary);
	arqEntrada.read((char*) &Cabecalho, sizeof(cabecalhoArqSS));	
	
	cout << Cabecalho.capacidadeMaxPacote << " " << Cabecalho.capacidadeMinPacote <<" "  << 
	Cabecalho.posicaoMeio << " " << Cabecalho.numPacotes << " ";
	cout << Cabecalho.posPrimeiroPacote <<" " << Cabecalho.proxPosicaoVazia; 
	
	unsigned posArq;
	
	ArvoreBMais minhaArvore("teste.dat");
	minhaArvore.atualizaEmOrdem();
	unsigned id; // chave da tabela 
	
	for (unsigned i = 1; i < Cabecalho.numPacotes; i++){
		posArq = sizeof(cabecalhoArqSS) + i*sizeof(pacote);
		arqEntrada.seekg(posArq);
		arqEntrada.read((char*) &umDado, sizeof(dado));
		minhaArvore.insere(umDado);
	}
	
	
	arqEntrada.close();
	
	cout << "Operacoes na Arvore : " << endl;
	
	do{
		try{
			cin >> operacao;
			
			switch (operacao){
				case 'p':{
					minhaArvore.percorreEmOrdem();
					
					break;
				}
				case 'b':{
					cin >> id;
					minhaArvore.Busca(id); 
	
					break;
				}
				case 'd':{
					cout << minhaArvore;
					
					break;
				}
				case 's':{
					break;
				}
				default:{
					cout << "Invalido\n";
				}
			}
		}
		catch (exception& e){
			cout << "Erro " << endl;
		}
	} while(operacao != 's');
	
	return 0;
}

