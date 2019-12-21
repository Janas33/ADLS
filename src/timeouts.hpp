#pragma once

struct TimeoutData {
	TimeoutData(void f(), const int &t, const char *n) : timeout(t), f(f), name(n) {}
	void(*f)();
	int timeout;
  const char *name;
};

/*
 * Funkcja do ustawiania timeoutu
 * @f() - tutaj funckja która ma się wykonać za zadany czas
 *       nie może ona przyjmować żadnych parametrów
 *       oraz musi zwracać void  
 */
void set_timeout(void f(), const int &time, const char *name);

/*
 * Funkcja sprawdzająca czy już czas na 
 * wykonanie jakiegoś timeoutu  
 */
void handle_timeouts();

/*
 * Funkcja usuwająca timeout po nazwie
 * usuwa pierwszy znaleziony o takiej nazwie
 * @name - nazwa timeoutu do usunięcia  
 */
void remove_timeout(const char *name);

/*
 * Funkcja odnajdująca timeout po nazwie  
 * znajduje pierwszy znaleziony o takiej nazwie
 * Zwraca ten timeout (TimeoutData)
 */
TimeoutData* find_timeout(const char *name);
