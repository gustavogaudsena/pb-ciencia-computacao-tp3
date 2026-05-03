class VerbeteNodo:
    def __init__(self, palavra, significado):
        self.palavra = palavra
        self.significado = significado
        self.esquerda = None
        self.direita = None
        self.altura = 1


class DicionarioDeVerbetes:
    def __init__(self):
        self.raiz = None
        self.total_itens = 0

    def _normalizar(self, palavra):
        return palavra.strip().lower()
    
    def __len__(self):
        return self.total_itens
    
    def altura(self):
        return self._get_altura(self.raiz)
    
    def _get_altura(self, no):
        if not no:
            return 0
        return no.altura

    def _get_fator_balanceamento(self, no):
        if not no:
            return 0
        return self._get_altura(no.esquerda) - self._get_altura(no.direita)
    

    def _rotacao_direita(self, no_pai: VerbeteNodo):
        no_filho_esquerda = no_pai.esquerda
        no_neto_direita = no_filho_esquerda.direita

        no_filho_esquerda.direita = no_pai
        no_pai.esquerda = no_neto_direita

        no_pai.altura = 1 + max(self._get_altura(no_pai.esquerda), self._get_altura(no_pai.direita))
        no_filho_esquerda.altura = 1 + max(self._get_altura(no_filho_esquerda.esquerda), self._get_altura(no_filho_esquerda.direita))

        return no_filho_esquerda

    def _rotacao_esquerda(self, no_pai: VerbeteNodo):
        no_filho_direita = no_pai.direita
        no_neto_esquerda = no_filho_direita.esquerda

        no_filho_direita.esquerda = no_pai
        no_pai.direita = no_neto_esquerda

        no_pai.altura = 1 + max(self._get_altura(no_pai.esquerda), self._get_altura(no_pai.direita))
        no_filho_direita.altura = 1 + max(self._get_altura(no_filho_direita.esquerda), self._get_altura(no_filho_direita.direita))

        return no_filho_direita
    
    def inserir(self, palavra, significado):
        self.raiz = self._inserir_recursivo(self.raiz, self._normalizar(palavra), significado)
        self.total_itens += 1
        
    def _inserir_recursivo(self, raiz, palavra, significado):
        if not raiz:
            return VerbeteNodo(palavra, significado)

        if palavra < raiz.palavra:
            raiz.esquerda = self._inserir_recursivo(raiz.esquerda, palavra, significado)
        elif palavra > raiz.palavra:
            raiz.direita = self._inserir_recursivo(raiz.direita, palavra, significado)
        else:
            raiz.significado = significado
            return raiz
        
        raiz.altura = 1 + max(self._get_altura(raiz.esquerda), self._get_altura(raiz.direita))

        fator_balanceamento = self._get_fator_balanceamento(raiz)

        if fator_balanceamento > 1 and palavra < raiz.esquerda.palavra:
            return self._rotacao_direita(raiz)

        if fator_balanceamento < -1 and palavra > raiz.direita.palavra:
            return self._rotacao_esquerda(raiz)

        if fator_balanceamento > 1 and palavra > raiz.esquerda.palavra:
            raiz.esquerda = self._rotacao_esquerda(raiz.esquerda)
            return self._rotacao_direita(raiz)

        if fator_balanceamento < -1 and palavra < raiz.direita.palavra:
            raiz.direita = self._rotacao_direita(raiz.direita)
            return self._rotacao_esquerda(raiz)

        return raiz
    
    def remover(self, palavra):
        if self.buscar(palavra) is None:
            return
        self.raiz = self._remover_recursivo(self.raiz, self._normalizar(palavra))
        self.total_itens -= 1
        
    def _remover_recursivo(self, raiz, palavra):
        if not raiz:
            return raiz

        if palavra < raiz.palavra:
            raiz.esquerda = self._remover_recursivo(raiz.esquerda, palavra)
        elif palavra > raiz.palavra:
            raiz.direita = self._remover_recursivo(raiz.direita, palavra)
        else:
            if not raiz.esquerda:
                return raiz.direita
            elif not raiz.direita:
                return raiz.esquerda
            
            temp = self._no_valor_minimo(raiz.direita)
            raiz.palavra = temp.palavra
            raiz.significado = temp.significado
            raiz.direita = self._remover_recursivo(raiz.direita, temp.palavra)

        raiz.altura = 1 + max(self._get_altura(raiz.esquerda), self._get_altura(raiz.direita))

        fator_balanceamento = self._get_fator_balanceamento(raiz)

        if fator_balanceamento > 1 and self._get_fator_balanceamento(raiz.esquerda) >= 0:
            return self._rotacao_direita(raiz)

        if fator_balanceamento < -1 and self._get_fator_balanceamento(raiz.direita) <= 0:
            return self._rotacao_esquerda(raiz)

        if fator_balanceamento > 1 and self._get_fator_balanceamento(raiz.esquerda) < 0:
            raiz.esquerda = self._rotacao_esquerda(raiz.esquerda)
            return self._rotacao_direita(raiz)

        if fator_balanceamento < -1 and self._get_fator_balanceamento(raiz.direita) > 0:
            raiz.direita = self._rotacao_direita(raiz.direita)
            return self._rotacao_esquerda(raiz)

        return raiz
            
    def _no_valor_minimo(self, raiz):
        while raiz.esquerda is not None:
            raiz = raiz.esquerda
        return raiz
    
    def buscar(self, palavra) -> str:
        return self._buscar_recursivo(self.raiz, self._normalizar(palavra))

    def _buscar_recursivo(self, raiz: VerbeteNodo, palavra):
        if not raiz:
            return None
        if palavra == raiz.palavra:
            return raiz.significado
        elif palavra < raiz.palavra:
            return self._buscar_recursivo(raiz.esquerda, palavra)
        else:
            return self._buscar_recursivo(raiz.direita, palavra)
    
    def _percorrer_em_ordem(self, no):
        if not no:
            return []
        return (self._percorrer_em_ordem(no.esquerda) + [no.palavra] + self._percorrer_em_ordem(no.direita))

    def visualizar(self):
        return self._percorrer_em_ordem(self.raiz)
    
    def exibir(self, no_atual=None, nivel=0):
        if no_atual is None and nivel == 0:
            no_atual = self.raiz
            
        if no_atual is not None:
            self.exibir(no_atual.direita, nivel + 1)
            print(' ' * 4 * nivel + f'--> {no_atual.palavra}')
            self.exibir(no_atual.esquerda, nivel + 1)
            


print (" TESTE DE DICIONÁRIO DE VERBETES ".center(50, '-'))
dicionario = DicionarioDeVerbetes()
verbetes = [
    ("casa", "Construção destinada à habitação"),
    ("livro", "Conjunto de folhas impressas e encadernadas"),
    ("arvore", "Planta lenhosa de grande porte"),
    ("computador", "Máquina eletrônica para processar dados"),
    ("musica", "Arte de combinar sons de forma harmônica"),
    ("escola", "Instituição de ensino"),
    ("amigo", "Pessoa com quem se tem laço de afeto"),
    ("agua", "Líquido incolor essencial à vida"),
    ("sol", "Estrela central do sistema solar"),
    ("lua", "Satélite natural da Terra"),
    ("cidade", "Aglomeração urbana de grande porte"),
    ("praia", "Faixa de areia à beira-mar"),
    ("flor", "Parte reprodutiva das plantas"),
    ("janela", "Abertura na parede para luz e ar"),
    ("porta", "Estrutura para entrada e saída de ambientes"),
    ("rua", "Via pública em áreas urbanas"),
    ("carro", "Veículo automotor de quatro rodas"),
    ("trem", "Veículo ferroviário com vagões"),
    ("nuvem", "Massa visível de vapor de água na atmosfera"),
    ("estrela", "Corpo celeste que emite luz própria"),
    ("oceano", "Grande extensão de água salgada"),
    ("montanha", "Elevação natural do terreno"),
    ("biblioteca", "Local que reúne acervo de livros"),
    ("caderno", "Conjunto de folhas para escrita"),
]


for (v, s) in verbetes:
    dicionario.inserir(v, s)
    
dicionario.exibir()

print("\nPalavras contidas no dicionário:")
print(dicionario.visualizar())

print(f"\nSiginificado da palavra '{verbetes[2][0]}' ==> ", dicionario.buscar(verbetes[2][0]))

print("Altura da árvore: " + str(dicionario.altura()))
print("Total de verbetes: " + str(len(dicionario)))

print()
print(" TESTE DE INSERÇÕES ".center(50, '-'))
verbetes_extras = [
    ("rio", "Curso natural de água que deságua em outro corpo d'água"),
    ("floresta", "Grande extensão coberta por árvores"),
    ("vento", "Movimento de massas de ar na atmosfera"),
]

for (v, s) in verbetes_extras:
    print(f"Inserindo verbete: '{v}'")
    dicionario.inserir(v, s)
    
print()
dicionario.exibir()
print()
print("Altura da árvore: " + str(dicionario.altura()))
print("Total de verbetes: " + str(len(dicionario)))
print()
print(" TESTE DE REMOÇÕES ".center(50, '-'))
indices_remover = [7, 6, 2, 19]
for i in indices_remover:
    print(f"Removendo verbete: '{verbetes[i][0]}'")   
    dicionario.remover(verbetes[i][0])

print()
dicionario.exibir()
print()
print("Altura da árvore após remoções: " + str(dicionario.altura()))
print("Total de verbetes após remoções: " + str(len(dicionario)))


