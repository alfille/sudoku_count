import sys
import tkinter as tk
import tkinter.font as tkfont
import argparse

class Sudoku(tk.Frame):
	SUBSIZE = 3
	color=["dark gray","white"]
	
	def __init__(self, master=None, subsize=3):
		super().__init__(master)
		self.SUBSIZE = subsize
		self.SIZE = self.SUBSIZE * self.SUBSIZE
		self.TOTALSIZE = self.SIZE * self.SIZE
		self.master = master
		self.font = tkfont.Font(weight="bold",size=14)
		#self.pack()
		self.create_widgets()
		self.create_menu()

	def set_square(self,i,j,n):
		self.but[i][j].configure(text=n)
		self.sq_popup_done( i , j )
		
	def sq_popup_done( self, i, j ):
		self.pop.destroy()
		self.but[i][j].configure(relief="raised")
			
	
	def sq_popup( self,i,j):
		print("Pressed: "+repr(i)+" "+repr(j)+" \n")
		self.but[i][j].configure(relief="sunken")
		self.pop=tk.Toplevel()
		for si in range(self.SUBSIZE):
			for sj in range(self.SUBSIZE):
				n = si*self.SUBSIZE+sj
				tk.Button(self.pop,text=str(n),borderwidth=4,height=2,width=3,font=self.font,command=lambda i=i,j=j,n=str(n): self.set_square(i,j,n)).grid(row=si,column=sj)
		tk.Button(self.pop,text="Clear",borderwidth=4,height=2,font=self.font,command=lambda i=i,j=j,n=" ": self.set_square(i,j,n)).grid(columnspan=self.SUBSIZE,sticky="EW")
		tk.Button(self.pop,text="Back",borderwidth=4,height=2,font=self.font,command=lambda i=i,j=j: self.sq_popup_done(i,j)).grid(columnspan=self.SUBSIZE,sticky="EW")
#		self.pop.pack()

	def create_widgets(self):
		self.win = tk.Grid()
		self.but = [[0 for i in range(self.SIZE)] for j in range(self.SIZE)]
		for si in range(self.SUBSIZE):
			for sj in range(self.SUBSIZE):
				f = tk.Frame(self.master,background=self.color[(si+sj)%2],borderwidth=2,relief="flat")
				f.grid(row=si,column=sj)
				for ssi in range(self.SUBSIZE):
					for ssj in range(self.SUBSIZE):
						i = si*self.SUBSIZE+ssi
						j = sj*self.SUBSIZE+ssj
						self.but[i][j] = tk.Button(f,text=str(1+(i+j)%self.SIZE),borderwidth=4,height=2,width=3,font=self.font,command=lambda i=i,j=j: self.sq_popup(i,j))
						self.but[i][j].grid(row=ssi, column=ssj)
		#self.win = tk.Button(self)
		#self.win["text"] = "Hello World\n(click me)"
		#self.win["command"] = self.say_hi
		#self.win.pack(side="top")

		#self.quit = tk.Button(self, text="QUIT", fg="red",
		#					  command=self.master.destroy)
		#self.quit.pack(side="bottom")
		
	def choose(self):
		print
	
	def about(self):
		print("Sudoku Solve by Paul Alfille 2020")
	
	def create_menu(self):
		self.menu = tk.Menu(self.master,tearoff=0)
		self.helpmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Help",menu=self.helpmenu)
		self.helpmenu.add_command(label="About",command=self.about)
		self.master.config(menu=self.menu)

	def say_hi(self):
		print("hi there, everyone!")

def main(args):
	root = tk.Tk()
	app = Sudoku(master=root)
	app.mainloop()

if __name__ == "__main__":
	# execute only if run as a script
	sys.exit(main(sys.argv))
