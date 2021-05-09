from tkinter import * 
from tkinter import messagebox
from tkinter import Canvas
import subprocess

root = Tk()
root.geometry("300x300")
root.title("DPTrees")

#Frames for overall layout
topframe = Frame(root)

#Scrollbars
scrollbary = Scrollbar(root, orient=VERTICAL)
# scrollbary = Scrollbar(bottomframe)
canvas = Canvas(root, yscrollcommand=scrollbary.set) #bg="blue")
# canvas = Canvas(bottomframe, yscrollcommand=scrollbary.set)
scrollbary.config(command=canvas.yview)
canvas.configure(scrollregion=canvas.bbox("all"))
# canvas = Canvas(root) #bg="blue")
scrollbary.pack(side=RIGHT, fill=Y)

#List of entry fields
#Each contains a statement entered by the user
entries = []
statements = []

#LOGICAL SYMBOLS
#AND OTHER CONSTANTS
NOT = '\u00ac'
AND = '\u2227'
OR = '\u2228'
IF = '\u2192' 
IFF = '\u2194'
TRUE = "[True]"
FALSE = "[False]"

#------------------------------
#------ Button functions ------
#------------------------------

def addstatementfunc(): 
	global entries
	e = Entry(root)
	e.pack(in_=topframe, side=TOP) #Add statements to top of window
	e.bind("<Key>", lambda event: keyPress(event, e))
	entries.append(e)
    # e.pack(side=BOTTOM)
    
def clearbbuttonfunc():
    global entries
    global statements
    for e in entries:
        e.pack_forget()
        e.unbind("<Key>")
		
    entries = []
    statements = []
    canvas.delete("all")
    
def dpbuttonfunc(docnf):
    global statements
    global entries
    global dpbutton
    statements = [] 
    canvas.delete("all") #Clear the canvas
    addstatement.config(state=DISABLED)
    dpbutton.config(state=DISABLED) #Disable button to avoid issues
    dpcnfbutton.config(state=DISABLED)
    clearbutton.config(state=DISABLED)
    if len(entries) == 0: #If there are no entries, then ignore
        dpbutton.config(state=NORMAL) #reset dpbutton state
        addstatement.config(state=NORMAL)
        dpcnfbutton.config(state=NORMAL)
        clearbutton.config(state=NORMAL)
        return
    for e in entries:
        if e.get() != "":
            statements.append(e.get()) #Get each statement, store in list
        
    if len(statements) == 0: #If the entries have nothing in them, ignore this
        dpbutton.config(state=NORMAL)
        addstatement.config(state=NORMAL)
        dpcnfbutton.config(state=NORMAL)
        clearbutton.config(state=NORMAL)
        return
        
    outpt = ""
    if docnf: #IF we're doing CNF, let the other program know
        outpt = "-cnf\n"
        
    for s in statements: #Process each statement, print it
        s = s.replace(NOT, "!")
        s = s.replace(AND, "&")
        s = s.replace(OR, "|")
        s = s.replace(IF, "$")
        s = s.replace(IFF, "%")
        outpt = outpt + s + "; "
        
    # print("\n\0")
    outpt = outpt + "\n0"
	
    p = subprocess.run(["dp"], capture_output=True, text=True, input=outpt)
	
	#------------------------------------------------------------------
	
	#Read resulting input statements
	# which describe the tree
    lines = p.stdout.split("\n")
    # print(lines)
    
    nodes = []
    branches = []
    for i in range(len(lines)):
        s = lines[i]
        
        if s == "end":
            break
        
        if s[:5] == "Error": #ERror message
            messagebox.showerror("Logic Error", s) #Do some messagebox stuff
            dpbutton.config(state=NORMAL) #Reset dpbutton state
            addstatement.config(state=NORMAL)
            dpcnfbutton.config(state=NORMAL)
            clearbutton.config(state=NORMAL)
            return
        
        if  len(s) >= 2 and s[0] == " " and s[1] == "#": #REmove space at beginning
            s = s[1:]
            
        if  len(s) >= 2 and s[-1] == " " and s[-2] != "#": #Remove space at end
            s = s[:-1]
        
        if i % 2 == 0:
            nodes.append(s)
        else:
            branches.append(s)
	
    if not docnf: #Draw tree using non-cnf style
        scalex = .5
        scaley = .6 #Do I need this?
        deltax = max(40 * len(nodes), (2 ** len(nodes)) * 10)
        deltay = max(20 * len(nodes), (2 ** len(nodes)) * 5)
        x_init = deltax * (1 / (1 - scalex))
        y_init = 15
        xvals = [x_init]
        yvals = [y_init]
        canvas.config(width=2*x_init, height=2*deltay*(1 / (1 - scaley)))
        
        for i in range(0, len(nodes)):
            #First, draw the nodes for the tree
            curnodes = nodes[i] #Nodes for current level of tree
            curnodes = key2symb(curnodes)
            s = curnodes.split("#") 
            newxvals = []
            newyvals = []
            coord_counter = 0
            # print(s)
            for ss in s:
                if ss != "":
                    sss = ss.split()
                    for ssss in sss: #Draw one node
                        if ssss != []:
                            # print(str(coord_counter) + " " + ssss)
                            canvas.create_text(xvals[coord_counter], yvals[coord_counter], anchor = N, text=ssss)
                            yvals[coord_counter] = yvals[coord_counter] + 15
							
                    newxvals.append(xvals[coord_counter] - deltax)
                    newxvals.append(xvals[coord_counter] + deltax)
                    newyvals.append(yvals[coord_counter] + deltay)
                    newyvals.append(yvals[coord_counter] + deltay)
                    coord_counter = coord_counter + 1
                    
					
            #Draw branches
            labelscale = .8
            labelshift = 12
            if i != len(nodes) - 1:
                thislayer = branches[i]
                for j,branch in enumerate(thislayer.split()):
                    if branch != "-" and branch != "":
                        canvas.create_line(xvals[int(j/2)], yvals[int(j/2)], newxvals[j], newyvals[j], width=2)
                        canvas.create_text(int((xvals[int(j/2)] + newxvals[j])/2) + ((-1)**(j+1) * labelshift), int((yvals[int(j/2)] + newyvals[j])/2) - labelshift, text=key2symb(branch[1:]))
                        labelshift = max(labelshift * labelscale, 8)
            
            deltax = deltax * scalex
            deltay = deltay * scaley
            xvals = newxvals
            yvals = newyvals
    else:
        scalex = .5
        scaley = .6 #Do I need this?
        deltax = max(40 * len(nodes), (2 ** len(nodes)) * 10)
        deltay = max(20 * len(nodes), (2 ** len(nodes)) * 5)
        x_init = deltax * (1 / (1 - scalex))
        y_init = 15
        xvals = [x_init]
        yvals = [y_init]
        canvaswidth = 2*x_init
        canvasheight = 2*deltay*(1 / (1 - scaley))
        canvas.config(width=2*x_init, height=2*deltay*(1 / (1 - scaley)))
        
        for i in range(0, len(nodes)):
            #First, draw the nodes for the tree
            curnodes = nodes[i] #Nodes for current level of tree
            curnodes = key2symb(curnodes)
            s = curnodes.split("#") 
            newxvals = []
            newyvals = []
            coord_counter = 0
            # print(s)
            for ss in s:
                if ss != "": 
                # if coord_counter < (2**i):
                    sss = ss.split()
                    for ssss in sss: #Draw one node
                        if ssss != []:
                            # print(str(coord_counter) + " " + ssss)
                            if ssss[0] == ">":
                                canvas.create_line(xvals[coord_counter], yvals[coord_counter], xvals[coord_counter], yvals[coord_counter] + 40, width=2)
                                canvas.create_text(xvals[coord_counter] + 35, yvals[coord_counter] + 15, anchor=N, text=ssss)
                                yvals[coord_counter] = yvals[coord_counter] + 40 
                                canvasheight = canvasheight + 200
                                canvas.config(width=canvaswidth, height=canvasheight)
                            else:
                                canvas.create_text(xvals[coord_counter], yvals[coord_counter], anchor = N, text=ssss)
                                yvals[coord_counter] = yvals[coord_counter] + 15
							
                    newxvals.append(xvals[coord_counter] - deltax)
                    newxvals.append(xvals[coord_counter] + deltax)
                    newyvals.append(yvals[coord_counter] + deltay)
                    newyvals.append(yvals[coord_counter] + deltay)
                    coord_counter = coord_counter + 1
                    
            while coord_counter < (2**i):
                newxvals.append(xvals[coord_counter] - deltax)
                newxvals.append(xvals[coord_counter] + deltax)
                newyvals.append(yvals[coord_counter] + deltay)
                newyvals.append(yvals[coord_counter] + deltay)
                coord_counter = coord_counter + 1 #This loop fixes some issue with indexoutofbounds
                    
					
            #Draw branches
            labelscale = .8
            labelshift = 12
            if i != len(nodes) - 1:
                thislayer = branches[i]
                for j,branch in enumerate(thislayer.split()):
                    if branch != "-" and branch != "":
                        canvas.create_line(xvals[int(j/2)], yvals[int(j/2)], newxvals[j], newyvals[j], width=2)
                        canvas.create_text(int((xvals[int(j/2)] + newxvals[j])/2) + ((-1)**(j+1) * labelshift), int((yvals[int(j/2)] + newyvals[j])/2) - labelshift, text=key2symb(branch[1:]))
                        labelshift = max(labelshift * labelscale, 8)
            
            deltax = deltax * scalex
            deltay = deltay * scaley
            xvals = newxvals
            yvals = newyvals
    
	
    dpbutton.config(state=NORMAL) #Re-enable button
    addstatement.config(state=NORMAL)
    dpcnfbutton.config(state=NORMAL)
    clearbutton.config(state=NORMAL)
        
def helpbuttonfunc():
    messagebox.showinfo("Help", "! - " + NOT + "\n& - " + AND + "\n| - " + OR + "\n$ - " + IF + "\n% - " + IFF)
    
	
#Helper function to replace keyboard 
#symbols with logic symbols
def key2symb(s):
    s = s.replace("!", NOT)
    s = s.replace("&", AND)
    s = s.replace("|", OR)
    s = s.replace("$", IF)
    s = s.replace("%", IFF)
    s = s.replace(FALSE, "False")
    s = s.replace(TRUE, "True")
    s = s.replace("*", "False")
    s = s.replace("+", "True")
    return s
	
#When a key is pressed, override with corresponding logical 
#symbol
def keyPress(event, entry):
	symb = event.char
	if symb == '!':
		entry.insert(END, NOT)
		return 'break'
	elif symb == '&':
		entry.insert(END, AND)
		return 'break'
	elif symb == '|':
		entry.insert(END, OR)
		return 'break'
	elif symb == '$':
		entry.insert(END, IF)
		return 'break'
	elif symb == '%':
		entry.insert(END, IFF)
		return 'break'
        
#When the window is closed, do this
def close_func(): 
	# print("END")
	root.quit()
	root.destroy()
	

#--------------------------
#------ Create stuff ------
#--------------------------

#Buttons
topbuttonwidth = 12
topbuttonheight = 1
addstatement = Button(root, text="Add Statement", width=topbuttonwidth, height=topbuttonheight, command=addstatementfunc)
dpbutton = Button(root, text="Do DP!", width=topbuttonwidth, height=topbuttonheight, command=lambda:dpbuttonfunc(False))
dpcnfbutton = Button(root, text="Do DP! (CNF)", width=topbuttonwidth, height=topbuttonheight, command=lambda:dpbuttonfunc(True))
clearbutton = Button(root, text="Clear all", width=topbuttonwidth, height=topbuttonheight, command=clearbbuttonfunc)
helpbutton = Button(root, text="Help", width=topbuttonwidth, height=topbuttonheight, command=helpbuttonfunc);

#---------------------------------
#------ Add stuff to window ------
#---------------------------------

#Add frames
topframe.pack(side=TOP)

#Add buttons
addstatement.pack(in_=topframe, side=TOP)
dpbutton.pack(in_=topframe, side=TOP)
dpcnfbutton.pack(in_=topframe, side=TOP)
clearbutton.pack(in_=topframe, side=TOP)
helpbutton.pack(in_=topframe, side=TOP);

#Add canvas
canvas.pack(side=TOP, expand=True)

root.protocol("WM_DELETE_WINDOW", close_func) #When the window is closed, let other program know
root.mainloop()
